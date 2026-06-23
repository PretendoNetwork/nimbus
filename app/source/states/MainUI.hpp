#pragma once

#include "../common.hpp"

namespace MainUI
{
    Result unloadAccount(MainStruct* mainStruct);
    Result switchAccounts(MainStruct* mainStruct, u8 friend_account_id);
    Result createAccount(MainStruct* mainStruct, u8 friend_account_id, NascEnvironment environmentId);
    Result handleAzahar(u8 friend_account_id);
    void migrateAccount(MainStruct *mainStruct);
    void unlinkPNID(MainStruct *mainStruct);
    void launchPlugin(MainStruct *mainStruct);
    void openPrompt(MainStruct* mainStruct, const std::string& message, PromptStatus promptStatus);
    void updatePrompt(MainStruct* mainStruct, u32 kDown);
    void drawPrompt(MainStruct* mainStruct);
    bool drawUI(MainStruct *mainStruct, C3D_RenderTarget* top_screen, C3D_RenderTarget* bottom_screen, u32 kDown, u32 kHeld, touchPosition touch);
}
