#include <array>
#include <format>
#include <vector>
#include <inttypes.h>
#include <malloc.h>
#include <string>
#include <jansson.h>
#include <3ds.h>
#include <sys/select.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include "MainUI.hpp"
#include "../repair.hpp"
#include "../sysmodules/acta.hpp"
#include "../sysmodules/httpc.hpp"
#include "../plgldr.h"

constexpr Result ResultFPDLocalAccountNotExists = 0xC880C4ED; // FPD::LocalAccountNotExists
const char *NIMBUS_PLUGIN = "/luma/plugins/nimbus.3gx";
const char *NIMBUS_PLUGIN_MAGIC = "NMBS";
constexpr u32 NIMBUS_PLUGIN_VERSION = SYSTEM_VERSION(1, 0, 0);

Result retPNID = 0;
u32 pnidAccountSlot = 0;
AccountId pnid = {};

Result MainUI::unloadAccount(MainStruct *mainStruct) {
    Result rc = 0;

    handleResult(ACTA_UnloadConsoleAccount(), mainStruct, "Unload ACT account");
    if (R_FAILED(rc)) {
        return rc;
    }

    // In order to unload the friends account we need to make it go offline. Enter into exclusive state to disconnect from online
    handleResult(NDMU_EnterExclusiveState(NDM_EXCLUSIVE_STATE_LOCAL_COMMUNICATIONS), mainStruct, "Enter exclusive state");
    if (R_FAILED(rc)) {
        return rc;
    }

    bool online;
    // If the console was connected to online, wait until it disconnects.
    // I tried doing this through notification events but it didn't seem to work
    while (true) {
        handleResult(FRD_IsOnline(&online), mainStruct, "Online check");
        if (R_FAILED(rc)) {
            return rc;
        }

        if (!online) break;
    }

    // Now that the console is offline, unload the account
    handleResult(FRDA_UnloadLocalAccount(), mainStruct, "Unload friends account");

    return rc;
}

Result MainUI::switchAccounts(MainStruct *mainStruct, u8 friend_account_id) {
    Result rc = 0;

    handleResult(FRDA_LoadLocalAccount(friend_account_id), mainStruct, "Switch account");
    if (R_FAILED(rc)) {
        return rc;
    }

    u32 act_account_index = 0;
    handleResult(ACT_GetAccountIndexOfFriendAccountId(&act_account_index, friend_account_id), mainStruct, "Get ACT account ID of friend account ID");
    if (R_FAILED(rc)) {
        return rc;
    }

    if (act_account_index == 0) {
        u32 account_count;
        handleResult(ACT_GetAccountCount(&account_count), mainStruct, "Get account count");
        if (R_FAILED(rc)) {
            return rc;
        }

        handleResult(ACTA_CreateConsoleAccount(), mainStruct, "Create ACT account");
        if (R_FAILED(rc)) {
            return rc;
        }

        act_account_index = account_count + 1;

        handleResult(ACTA_CommitConsoleAccount(act_account_index), mainStruct, "Commit ACT account");
        if (R_FAILED(rc)) {
            return rc;
        }
    }

    handleResult(ACTA_SetDefaultAccount(act_account_index), mainStruct, "Set default account");

    return rc;
}

Result MainUI::createAccount(MainStruct *mainStruct, u8 friend_account_id, NascEnvironment environmentId) {
    Result rc = 0;

    // (Re)Create the friend account
    handleResult(FRDA_CreateLocalAccount(friend_account_id, static_cast<u8>(environmentId), 0, 1), mainStruct, "Create account");
    if (R_FAILED(rc)) {
        return rc;
    }

    // Switch to the friend/act accounts
    handleResult(switchAccounts(mainStruct, friend_account_id), mainStruct, "Switch account");
    if (R_FAILED(rc)) {
        return rc;
    }

    // Reset the act account
    handleResult(ACTA_UnbindServerAccount(friend_account_id, true), mainStruct, "Reset account");

    return rc;
}

Result MainUI::handleAzahar(u8 friend_account_id) {
    s64 emu_id = 0;
    svcGetSystemInfo(&emu_id, 0x20000, 0);

    if (emu_id != 2) {
        // This is not Azahar, nothing to do
        return 0;
    }

    std::array<std::string, 3> patterns = {{
        {"nintendowifi\\.net"},
        {"nintendo\\.net"},
        {"pokemon-gl\\.com"}
    }};
    const std::string replacement = "pretendo.cc";

    Result res = httpcInit(0x1000);
    if (friend_account_id == 2) {
        // Register Pretendo replacement URLs
        if (R_SUCCEEDED(res)) {
            for (const auto& pattern : patterns) {
                res = HTTPC_Azahar_RegisterURLReplacement(pattern.c_str(), replacement.c_str(), pattern.size(), replacement.size());
                if (R_FAILED(res)) {
                    // If the rule already exists clear it and register again
                    if (res == MAKERESULT(RL_STATUS, RS_INVALIDARG, RM_HTTP, RD_ALREADY_EXISTS)) {
                        res = HTTPC_Azahar_UnregisterURLReplacement(pattern.c_str(), pattern.size());
                        if (R_FAILED(res)) {
                            break;
                        }
                        res = HTTPC_Azahar_RegisterURLReplacement(pattern.c_str(), replacement.c_str(), pattern.size(), replacement.size());
                        if (R_FAILED(res)) {
                            break;
                        }
                    } else {
                        break;
                    }
                }
            }
        }
    } else {
        // Unregister Pretendo replacement URLs
        for (const auto& pattern : patterns) {
            res = HTTPC_Azahar_UnregisterURLReplacement(pattern.c_str(), pattern.size());
            if (R_FAILED(res)) {
                break;
            } else {
                // Result may have a non-failure info value indicating that the pattern
                // was not registered, clear it.
                res = 0;
            }
        }
    }

    if (res == MAKERESULT(RL_PERMANENT, RS_WRONGARG, RM_OS, 47)) {
        // LLE HTTP was enabled, so custom command didn't exist
        // the HTTP patches will be used instead, so ignore error.
        res = 0;
    }

    httpcExit();
    return res;
}

// Returns true if the app should exit and reboot straight away. On failure it returns false
// so that the error stays on screen until the user presses START to reboot.
bool MainUI::switchNetwork(MainStruct *mainStruct) {
    // Clear any previous logs
    mainStruct->errorString[0] = 0;

    u8 accountId = (u8)mainStruct->buttonSelected + 1; // by default set accountId to nasc environment + 1

    Result rc = unloadAccount(mainStruct);
    if (R_SUCCEEDED(rc)) {
        rc = switchAccounts(mainStruct, accountId);
        if (rc == ResultFPDLocalAccountNotExists && accountId == 2) {
            // Clear the error to allow createAccount to override it
            memset(mainStruct->errorString, 0, 256);
            rc = createAccount(mainStruct, accountId, NascEnvironment::NASC_ENV_Test);
        }
    }

    if (R_SUCCEEDED(rc)) {
        rc = handleAzahar(accountId);
        LOG_NIMBUS_ERROR(mainStruct, std::format("Failed to apply Azahar configuration: {}", rc).c_str());
    }

    mainStruct->needsReboot = true;

    if (R_FAILED(rc)) {
        aptSetHomeAllowed(false);
        return false;
    }

    return true;
}

void MainUI::migrateAccount(MainStruct *mainStruct) {
    Result rc = 0;
    u32 pretendo_account_index = 0;
    // Logs won't override any previous errors
    handleResult(ACT_GetAccountIndexOfFriendAccountId(&pretendo_account_index, 2), mainStruct, "Get PNID for migration");
    if (pretendo_account_index != 0) {
        bool is_commited = false;
        handleResult(ACT_GetAccountInfo(&is_commited, sizeof(bool), pretendo_account_index, INFO_TYPE_IS_COMMITTED), mainStruct, "Get PNID commit status");
        if (!is_commited) {
            handleResult(ACTA_CommitConsoleAccount(pretendo_account_index), mainStruct, "Commit PNID");
        }
    }
}

void MainUI::unlinkPNID(MainStruct *mainStruct) {
    if (R_FAILED(retPNID = ACTA_UnbindServerAccount(pnidAccountSlot, true))) {
        LOG_NIMBUS_ERROR(mainStruct, std::format("ACTA_UnbindServerAccount failed with error code {}!", retPNID).c_str());
	} else {
		LOG_NIMBUS_ERROR(mainStruct, "Successfully unlinked PNID!");
	}
}

// Borrowed from https://github.com/azahar-emu/ArticBaseServer/blob/d93be050a4787ed602c603aa14bbaaee066dc1d9/app/sources/main.cpp#L79
void MainUI::launchPlugin(MainStruct *mainStruct) {
    Result rc = 0;
    PluginLoadParameters plgparam = { 0 };
    bool isPlgEnabled = false;

    // Ideally we wouldn't have to set the persistent flag and instead use the builtin
    // functionality, but the Old 3DS does a reboot before opening a Mode3 game,
    // erasing any non-persistent configurations we use since this as a generic launcher.
    //
    // Instead, we have to mark the parameters as persistent and then disable them
    // from the plugin itself
    plgparam.noFlash = false;
    plgparam.pluginMemoryStrategy = PLG_STRATEGY_SWAP;
    plgparam.persistent = 1;
    plgparam.lowTitleId = 0;
    strcpy(plgparam.path, NIMBUS_PLUGIN);

    // Use custom header on config as a way to differentiate between plugin load from launcher
    // and load by the user (by saving the plugin file as default or on a specific game)
    strcpy(reinterpret_cast<char*>(plgparam.config), NIMBUS_PLUGIN_MAGIC);
    plgparam.config[1] = NIMBUS_PLUGIN_VERSION;

    handleResult(plgLdrInit(), mainStruct, "Initialize plg:ldr");
    if (R_FAILED(rc)) {
        return;
    }

    u32 version;
    handleResult(PLGLDR__GetVersion(&version), mainStruct, "Get plg:ldr version");
    if (R_FAILED(rc)) {
        plgLdrExit();
        return;
    }

    if (version < SYSTEM_VERSION(1, 0, 2)) {
        LOG_NIMBUS_ERROR(mainStruct, "Unsupported plg:ldr version, please update Luma3DS");
        plgLdrExit();
        return;
    }

    // Save the previous plugin loader state
    handleResult(PLGLDR__IsPluginLoaderEnabled(&isPlgEnabled), mainStruct, "Get plugin loader state");
    if (R_FAILED(rc)) {
        plgLdrExit();
        return;
    }
    plgparam.config[2] = isPlgEnabled;

    handleResult(PLGLDR__SetPluginLoaderState(true), mainStruct, "Enable plugin loader");
    if (R_FAILED(rc)) {
        plgLdrExit();
        return;
    }

    handleResult(PLGLDR__SetPluginLoadParameters(&plgparam), mainStruct, "Set plugin load params");
    plgLdrExit();

    // Logs won't override any previous errors
    LOG_NIMBUS_ERROR(mainStruct, "Nimbus plugin ready! Launch a game from the Home Menu");
    return;
}

// Taken from https://gist.github.com/litefeel/1197e5c24eb9ec93d771
void hexchar(unsigned char c, unsigned char &hex1, unsigned char &hex2)
{
    hex1 = c / 16;
    hex2 = c % 16;
    hex1 += hex1 <= 9 ? '0' : 'a' - 10;
    hex2 += hex2 <= 9 ? '0' : 'a' - 10;
}

std::string urlencode(std::string s)
{
    const char *str = s.c_str();
    std::vector<char> v(s.size());
    v.clear();
    for (size_t i = 0, l = s.size(); i < l; i++)
    {
        char c = str[i];
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '-' || c == '_' || c == '.' || c == '!' || c == '~' ||
            c == '*' || c == '\'' || c == '(' || c == ')')
        {
            v.push_back(c);
        }
        else if (c == ' ')
        {
            v.push_back('+');
        }
        else
        {
            v.push_back('%');
            unsigned char d1, d2;
            hexchar(c, d1, d2);
            v.push_back(d1);
            v.push_back(d2);
        }
    }

    return std::string(v.cbegin(), v.cend());
}

// Taken from curl examples
struct memory {
    char *response;
    size_t size;
};

static size_t curlcb(char *data, size_t size, size_t nmemb, void *clientp)
{
    size_t realsize = nmemb;
    struct memory *mem = (struct memory *)clientp;

    char *ptr = (char *)realloc(mem->response, mem->size + realsize + 1);
    if(!ptr)
        return 0;  /* out of memory */

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

// Adapted from https://github.com/devkitPro/3ds-examples/blob/master/network/http_post/source/main.c
void MainUI::getNewHMAC(MainStruct *mainStruct, u32 pid, const char *password, std::string& pidHMAC)
{
    std::string url="https://api.pretendo.cc/v1/repair-uidhmac";
    u32 statuscode=0;
    struct memory chunk = { 0 };

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, std::format("{} {}.{}.{}", APP_TITLE, VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO).c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, std::format("pid={}&password={}", std::to_string(pid), urlencode(password)).c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlcb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    bool proxyEnabled = false;
    char proxyHost[0x100];
    u16 proxyPort;
    char proxyUsername[0x20];
    char proxyPassword[0x20];
    if (R_SUCCEEDED(ACU_GetProxyEnable(&proxyEnabled)) && proxyEnabled &&
        R_SUCCEEDED(ACU_GetProxyHost(proxyHost)) &&
        R_SUCCEEDED(ACU_GetProxyPort(&proxyPort)) &&
        R_SUCCEEDED(ACU_GetProxyUserName(proxyUsername)) &&
        R_SUCCEEDED(ACU_GetProxyPassword(proxyPassword))
    ) {
        curl_easy_setopt(curl, CURLOPT_PROXY, std::format("{}:{}", proxyHost, proxyPort).c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, proxyUsername);
        curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, proxyPassword);
    }

    CURLcode curlrc = curl_easy_perform(curl);
    if (curlrc != CURLE_OK) {
        LOGF_NIMBUS_ERROR(mainStruct, "curl_easy_perform failed: %s", curl_easy_strerror(curlrc));
        free(chunk.response);
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statuscode);

    if(statuscode!=200){
        LOGF_NIMBUS_ERROR(mainStruct, "cURL returned status: %" PRIx32, statuscode);
        free(chunk.response);
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    json_error_t janssonError = {};
    auto* jsonRoot = json_loadb(reinterpret_cast<char*>(chunk.response), chunk.size, 0, &janssonError);
    free(chunk.response);
    if (janssonError.text[0] != 0) {
        LOGF_NIMBUS_ERROR(mainStruct, "Jansson error: %s", janssonError.text);
        json_decref(jsonRoot);
        return;
    }

    if (!json_is_object(jsonRoot)) {
        LOG_NIMBUS_ERROR(mainStruct, "Unknown HTTP response");
        json_decref(jsonRoot);
        return;
    }

    auto* jsonStatus = json_object_get(jsonRoot, "status");
    if (!json_is_integer(jsonStatus)) {
        LOG_NIMBUS_ERROR(mainStruct, "Unknown JSON response");
        json_decref(jsonRoot);
        return;
    }

    auto jsonStatusValue = json_integer_value(jsonStatus);
    if (jsonStatusValue != 200) {
        auto* jsonError = json_object_get(jsonRoot, "error");
        LOGF_NIMBUS_ERROR(mainStruct, "API error: %s", json_is_string(jsonError) ? json_string_value(jsonError) : "Unknown JSON error response");
        json_decref(jsonRoot);
        return;
    }

    auto* jsonData = json_object_get(jsonRoot, "data");
    if (!json_is_object(jsonData)) {
        LOG_NIMBUS_ERROR(mainStruct, "Unknown JSON data response");
        json_decref(jsonRoot);
        return;
    }

    auto* jsonHMAC = json_object_get(jsonData, "uidhmac");
    if (!json_is_string(jsonHMAC)) {
        LOG_NIMBUS_ERROR(mainStruct, "Unknown JSON uidhmac response");
        json_decref(jsonRoot);
        return;
    }

    pidHMAC = json_string_value(jsonHMAC);
    json_decref(jsonRoot);
    return;
}

void MainUI::openPrompt(MainStruct* mainStruct, const std::string& message, PromptStatus promptStatus)
{
    mainStruct->prompt.active = true;
    mainStruct->prompt.message = message;
    mainStruct->prompt.result = PromptResult::None;
    mainStruct->prompt.status = promptStatus;
}

void MainUI::updatePrompt(MainStruct* mainStruct, u32 kDown)
{
    if (!mainStruct->prompt.active)
        return;

    // Only the network switch notice can be confirmed by tapping. Other prompts may be
    // destructive (e.g. unlinking a PNID), so a stray tap must not confirm them.
    const bool tapConfirms = mainStruct->prompt.status == PromptStatus::NetworkSwitch && (kDown & KEY_TOUCH);

    if ((kDown & KEY_A) || tapConfirms) {
        mainStruct->prompt.result = PromptResult::Yes;
    }
    else if (kDown & KEY_B) {
        mainStruct->prompt.result = PromptResult::No;
    }
}

void MainUI::drawPrompt(MainStruct* mainStruct)
{
    if (!mainStruct->prompt.active) return;

    const float screenW = 320.0f;
    const float screenH = 240.0f;

    // Dim background
    C2D_DrawRectSolid(0, 0, 0.1f, screenW, screenH, C2D_Color32(0, 0, 0, 140));

    const float boxW = 280.0f, boxH = 110.0f;
    const float boxX = (screenW - boxW) / 2.0f;
    const float boxY = (screenH - boxH) / 2.0f;

    const u32 fill   = C2D_Color32(25, 25, 25, 240);
    const u32 border = C2D_Color32(255, 255, 255, 255);
    const u32 white  = C2D_Color32(255, 255, 255, 255);

    // Box + border
    C2D_DrawRectSolid(boxX, boxY, 0.2f, boxW, boxH, fill);
    C2D_DrawRectSolid(boxX, boxY,               0.3f, boxW, 2, border);
    C2D_DrawRectSolid(boxX, boxY + boxH - 2.0f, 0.3f, boxW, 2, border);
    C2D_DrawRectSolid(boxX, boxY,               0.3f, 2, boxH, border);
    C2D_DrawRectSolid(boxX + boxW - 2.0f, boxY, 0.3f, 2, boxH, border);

    // Parse both strings in one buffer
    C2D_Text msgText, hintText;
    C2D_TextBufClear(textBuf);

    C2D_TextFontParse(&msgText, font, textBuf, mainStruct->prompt.message.c_str());
    C2D_TextOptimize(&msgText);

    const bool isNetworkSwitch = mainStruct->prompt.status == PromptStatus::NetworkSwitch;
    C2D_TextFontParse(&hintText, font, textBuf, isNetworkSwitch ? "A/Tap: OK    B: Cancel" : "A: Yes    B: Cancel");
    C2D_TextOptimize(&hintText);

    // Draw (z <= 1.0!!)
    C2D_DrawText(&msgText,
                 C2D_WithColor | C2D_WordWrap,
                 boxX + 10.0f, boxY + 10.0f, 0.4f,
                 0.55f, 0.55f, white,
                 boxW - 20.0f);

    C2D_DrawText(&hintText,
                 C2D_WithColor | C2D_AlignCenter,
                 screenW / 2.0f, boxY + boxH - 18.0f, 0.4f,
                 0.5f, 0.5f, white);
}

bool MainUI::drawUI(MainStruct *mainStruct, C3D_RenderTarget* top_screen, C3D_RenderTarget* bottom_screen, u32 kDown, u32 kHeld, touchPosition touch)
{
    C2D_SceneBegin(top_screen);
    DrawScamWarning();
    DrawVersionString();
    C2D_DrawSprite(&mainStruct->top);

    // Check if Nimbus has been updated
    if (!mainStruct->updateChecked) {
        mainStruct->updateChecked = true;
        if (auto* updateCheck = std::fopen(NIMBUS_UPDATE_PATH "/update.txt", "rb")) {
            std::fclose(updateCheck);
            DrawString(0.5f, C2D_Color32(255, 255, 0, 0xFF), "Updating Nimbus...\n\nIf you see this screen for over a minute, reboot your console and ensure you have an Internet connection", 0);

            migrateAccount(mainStruct);

            // If the migration has failed, don't do the update. In such case users need to contact support
            if (mainStruct->errorString[0] == 0) {
                mkdir("/luma", 0777);
                mkdir("/luma/sysmodules", 0777);
                std::remove("/luma/sysmodules/0004013000003202.ips");
                std::rename(NIMBUS_UPDATE_PATH "/0004013000003202.ips", "/luma/sysmodules/0004013000003202.ips");
                std::remove("/luma/sysmodules/0004013000003802.ips");
                std::rename(NIMBUS_UPDATE_PATH "/0004013000003802.ips", "/luma/sysmodules/0004013000003802.ips");
                std::remove("/luma/sysmodules/0004013000002902.ips");
                std::rename(NIMBUS_UPDATE_PATH "/0004013000002902.ips", "/luma/sysmodules/0004013000002902.ips");
                std::remove("/luma/sysmodules/0004013000002E02.ips");
                std::rename(NIMBUS_UPDATE_PATH "/0004013000002E02.ips", "/luma/sysmodules/0004013000002E02.ips");
                std::remove("/luma/sysmodules/0004013000002F02.ips");
                std::rename(NIMBUS_UPDATE_PATH "/0004013000002F02.ips", "/luma/sysmodules/0004013000002F02.ips");

                mkdir("/luma/titles", 0777);
                mkdir("/luma/titles/000400300000BC02", 0777);
                std::remove("/luma/titles/000400300000BC02/code.ips");
                std::rename(NIMBUS_UPDATE_PATH "/000400300000BC02.ips", "/luma/titles/000400300000BC02/code.ips");

                mkdir("/luma/titles/000400300000BD02", 0777);
                std::remove("/luma/titles/000400300000BD02/code.ips");
                std::rename(NIMBUS_UPDATE_PATH "/000400300000BD02.ips", "/luma/titles/000400300000BD02/code.ips");

                mkdir("/luma/titles/000400300000BE02", 0777);
                std::remove("/luma/titles/000400300000BE02/code.ips");
                std::rename(NIMBUS_UPDATE_PATH "/000400300000BE02.ips", "/luma/titles/000400300000BE02/code.ips");

                mkdir("/luma/plugins", 0777);
                std::remove("/luma/plugins/nimbus.3gx");
                std::rename(NIMBUS_UPDATE_PATH "/nimbus.3gx",           "/luma/plugins/nimbus.3gx");

                std::remove("/3ds/juxt-prod.pem");
                std::rename(NIMBUS_UPDATE_PATH "/juxt-prod.pem",        "/3ds/juxt-prod.pem");

                std::remove(NIMBUS_UPDATE_PATH "/update.txt");
            }

            // Logs won't override any previous errors
            LOG_NIMBUS_ERROR(mainStruct, "Nimbus has been updated!");

            aptSetHomeAllowed(false);
            mainStruct->needsReboot = true;
            mainStruct->buttonWasPressed = false;
            return false;
        }
    }

    // Handle the pidHMAC fix migration (except if an error occured previously)
    if (!mainStruct->repairChecked && (mainStruct->errorString[0] == 0 || strncmp(mainStruct->errorString, "Nimbus has been updated!", sizeof("Nimbus has been updated!")) == 0)) {
        mainStruct->repairChecked = true;
        u32 pretendo_account_index = 0;
        // Logs won't override any previous errors
        Result rc = 0;
        handleResult(ACT_GetAccountIndexOfFriendAccountId(&pretendo_account_index, 2), mainStruct, "Get PNID for repair check");
        if (pretendo_account_index == 0) { // If an account doesn't exist, we just create the dummy file. The friends server will provide the pidHMAC
            auto* pidHMACFixCheck = std::fopen("/3ds/nimbus/.pidhmac", "wb");
            std::fclose(pidHMACFixCheck);
        }

        if (auto* pidHMACFixCheck = std::fopen("/3ds/nimbus/.pidhmac", "rb"); !pidHMACFixCheck) {
            mainStruct->errorString[0] = 0; // Clear potential previous error
            // TODO - This thing isn't drawn when Nimbus required an update beforehand. How do we make this text render in that case?
            // For now, we put a hint on the upgrade screen about Internet requirements
            DrawString(0.5f, C2D_Color32(255, 255, 0, 0xFF), "Upgrading Pretendo account... Ensure you have an Internet connection", 0);
            Result rc = unloadAccount(mainStruct);
            if (R_SUCCEEDED(rc)) {
                rc = switchAccounts(mainStruct, 2);
                if (rc == ResultFPDLocalAccountNotExists) {
                    // Clear the error as it is not necessary
                    memset(mainStruct->errorString, 0, 256);

                    pidHMACFixCheck = std::fopen("/3ds/nimbus/.pidhmac", "wb");
                    std::fclose(pidHMACFixCheck);
                } else {
                    // Leave the exclusive state as we need HTTP communications
                    handleResult(NDMU_LeaveExclusiveState(), mainStruct, "Leave exclusive state");

                    if (R_SUCCEEDED(rc)) {
                        handleResult(acInit(), mainStruct, "acInit");
                    }

                    if (R_SUCCEEDED(rc)) {
                        handleResult(WaitConnectedInternet(), mainStruct, "WaitConnectedInternet");
                    }

                    FriendKey fkey = {};
                    if (R_SUCCEEDED(rc)) {
                        handleResult(FRD_GetMyFriendKey(&fkey), mainStruct, "Get friend account PID");
                    }

                    char password[17] = {};
                    if (R_SUCCEEDED(rc)) {
                        handleResult(FRD_GetMyPassword(password, sizeof(password)), mainStruct, "Get friend account password");
                    }

                    static u32 *SOC_buffer = NULL;
                    if (R_SUCCEEDED(rc)) {
                        // allocate buffer for SOC service
                        SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

                        if(SOC_buffer == NULL) {
                            rc = -1;
                            LOG_NIMBUS_ERROR(mainStruct, "memalign: failed to allocate");
                        }
                    }

                    // Now intialise soc:u service
                    if (R_SUCCEEDED(rc)) {
                        handleResult(socInit(SOC_buffer, SOC_BUFFERSIZE), mainStruct, "socInit");
                    }

                    std::string pidHMAC = "";
                    if (R_SUCCEEDED(rc)) {
                        getNewHMAC(mainStruct, fkey.principalId, password, pidHMAC);
                    }

                    socExit();
                    acExit();

                    if (!pidHMAC.empty()) {
                        if (repairAccountHMAC(mainStruct, pidHMAC.substr(0, 8).c_str())) {
                            pidHMACFixCheck = std::fopen("/3ds/nimbus/.pidhmac", "wb");
                            std::fclose(pidHMACFixCheck);
                        }
                    }
                }
            }

            // Logs won't override any previous errors
            LOG_NIMBUS_ERROR(mainStruct, "Nimbus has been updated!");

            aptSetHomeAllowed(false);
            mainStruct->needsReboot = true;
            mainStruct->buttonWasPressed = false;
            return false;
        } else {
            std::fclose(pidHMACFixCheck);
        }
    }

    // if start is pressed, exit to hbl/the home menu depending on if the app was launched from cia or 3dsx
    if (kDown & KEY_START) return true;

    updatePrompt(mainStruct, kDown);

    if (mainStruct->prompt.active) {
        if (mainStruct->prompt.result == PromptResult::Yes) {
            bool exit = false;
            switch (mainStruct->prompt.status) {
                case PromptStatus::PNIDUnlink:
                    unlinkPNID(mainStruct);
                    break;
                case PromptStatus::NetworkSwitch:
                    exit = switchNetwork(mainStruct);
                    break;
                default:
                    LOG_NIMBUS_ERROR(mainStruct, "Unknown prompt called.");
                    break;
            }
            mainStruct->prompt.result = PromptResult::None;
            mainStruct->prompt.active = false;
            return exit;
        }
        if (mainStruct->prompt.result == PromptResult::No) {
            mainStruct->prompt.result = PromptResult::None;
            mainStruct->prompt.active = false;
            return false;
        }
    }

    if (mainStruct->errorString[0] != 0) {
        DrawString(0.5f, 0xFFFFFFFF, std::format("{}{}", mainStruct->errorString, mainStruct->needsReboot ? "\n\nPress START to reboot the system" : ""), 0);
    }

    C2D_SceneBegin(bottom_screen);
    DrawControls();

    if (mainStruct->buttonSelected == NascEnvironment::NASC_ENV_Prod) {
        if (mainStruct->currentAccount == NascEnvironment::NASC_ENV_Prod) {
            C2D_DrawSprite(&mainStruct->nintendo_loaded_selected);
            C2D_DrawSprite(&mainStruct->pretendo_unloaded_deselected);
        }
        else {
            C2D_DrawSprite(&mainStruct->nintendo_unloaded_selected);
            C2D_DrawSprite(&mainStruct->pretendo_loaded_deselected);
        }
    }
    else if (mainStruct->buttonSelected == NascEnvironment::NASC_ENV_Test) {
        if (mainStruct->currentAccount == NascEnvironment::NASC_ENV_Test) {
            C2D_DrawSprite(&mainStruct->nintendo_unloaded_deselected);
            C2D_DrawSprite(&mainStruct->pretendo_loaded_selected);
        }
        else {
            C2D_DrawSprite(&mainStruct->nintendo_loaded_deselected);
            C2D_DrawSprite(&mainStruct->pretendo_unloaded_selected);
        }
    }
    C2D_DrawSprite(&mainStruct->header);
    drawPrompt(mainStruct);

    // Only allow user interaction when the system doesn't need a restart
    if (!mainStruct->needsReboot && !mainStruct->prompt.active) {
        // handle touch input
        if (kDown & KEY_TOUCH) {
            if ((touch.px >= 165 && touch.px <= 165 + 104) && (touch.py >= 59 && touch.py <= 59 + 113)) {
                mainStruct->buttonSelected = NascEnvironment::NASC_ENV_Prod;
                mainStruct->buttonWasPressed = true;
            }
            else if ((touch.px >= 49 && touch.px <= 49 + 104) && (touch.py >= 59 && touch.py <= 59 + 113)) {
                mainStruct->buttonSelected = NascEnvironment::NASC_ENV_Test;
                mainStruct->buttonWasPressed = true;
            }
        }
        else if (kDown & KEY_LEFT || kDown & KEY_RIGHT) {
            mainStruct->buttonSelected = mainStruct->buttonSelected == NascEnvironment::NASC_ENV_Test ? NascEnvironment::NASC_ENV_Prod : NascEnvironment::NASC_ENV_Test;
        }

        if (mainStruct->prompt.active) {
            return false;
        }

        if (kDown & KEY_A) {
            mainStruct->buttonWasPressed = true;
        }

        if (kDown & KEY_X) {
            // We need to confirm we actually even have a linked PNID.
	        if (R_SUCCEEDED(retPNID)) {
		        if (R_FAILED(retPNID = ACT_GetAccountIndexOfFriendAccountId(&pnidAccountSlot, 2))) {
			        LOG_NIMBUS_ERROR(mainStruct, std::format("ACT_GetAccountIndexOfFriendAccountId failed with error code {}!", retPNID).c_str());
		        }
	        }

            if (pnidAccountSlot == 0) {
                LOG_NIMBUS_ERROR(mainStruct, "There is no PNID linked on this console!");
            }

	        if (R_SUCCEEDED(retPNID)) {
		        if (R_FAILED(retPNID = ACT_GetAccountInfo(pnid, sizeof(pnid), pnidAccountSlot, INFO_TYPE_ACCOUNT_ID))) {
			        LOG_NIMBUS_ERROR(mainStruct, std::format("ACT_GetAccountInfo failed with error code {}!", retPNID).c_str());
		        }
	        }

            if (R_SUCCEEDED(retPNID)) {
		        if (pnid[0] != '\0') {
			        openPrompt(mainStruct, std::format("Are you sure you would like to unlink your PNID {}? Your PNID can be relinked at any time.", pnid), PromptStatus::PNIDUnlink);
		        } else {
			        LOG_NIMBUS_ERROR(mainStruct, "There is no PNID linked on this console!");
		        }
	        }
        }

        if (kDown & KEY_Y) {
            launchPlugin(mainStruct);
            mainStruct->buttonWasPressed = false;
            return false;
        }
    }

    if (mainStruct->buttonWasPressed) {
        mainStruct->buttonWasPressed = false;

        // If the chosen account is the one we are already logged into, exit without rebooting
        if (mainStruct->currentAccount == mainStruct->buttonSelected) return true;

        // Switching networks reboots the console, so let the user know before anything happens
        openPrompt(mainStruct, "The system will now reboot.", PromptStatus::NetworkSwitch);
    }

    return false;
}
