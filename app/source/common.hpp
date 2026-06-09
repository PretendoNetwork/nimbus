#pragma once

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#include "../build/version.hpp"
#include <citro2d.h>
#include <3ds.h>
#include <algorithm>
#include <string>

#define NIMBUS_UPDATE_PATH "/3ds/nimbus/update"

enum class NascEnvironment : u8 {
	NASC_ENV_Prod = 0, // nintendo
	NASC_ENV_Test = 1, // pretendo
	NASC_ENV_Dev = 2
};

enum class CFWSystemInfoField : s32 {
	FirmwareVersion = 0,
	CommitHash = 1,
	ConfigVersion = 2,
	ConfigBits = 3 // only go through 0-3 because we only need 0, 2, and 3
};

enum class LumaConfigBitIndex : s32 {
	AutobootEmunand = 0,
	ExternalFirmsAndModules = 1,
	GamePatching = 2 // only go through 0-2 because we only need 1 and 2
};

const int targetLumaVersion = 13;
const int GetSystemInfoCFW = 0x10000; // the type for Luma3DS' GetSystemInfo hook that returns CFW info
const u32 defaultColor = C2D_Color32(255, 255, 255, 0xFF);

enum class PromptResult {
    None,
    Yes,
    No
};

enum class PromptStatus {
	Unknown,
    PNIDUnlink
};

struct PromptState {
    bool active = false;
    std::string message;
    PromptResult result = PromptResult::None;
	PromptStatus status = PromptStatus::Unknown;
};

struct MainStruct {
	C2D_Sprite debug_button;
	C2D_Sprite debug_header;
	C2D_Sprite go_back;
	C2D_Sprite header;
	C2D_Sprite nintendo_unloaded_deselected;
	C2D_Sprite nintendo_unloaded_selected;
	C2D_Sprite nintendo_loaded_selected;
	C2D_Sprite nintendo_loaded_deselected;
	C2D_Sprite pretendo_unloaded_deselected;
	C2D_Sprite pretendo_unloaded_selected;
	C2D_Sprite pretendo_loaded_selected;
	C2D_Sprite pretendo_loaded_deselected;
	C2D_Sprite top;

	u32 screen = 0;
	u32 state = 0;
	u32 lastState = 0;

	NascEnvironment currentAccount = NascEnvironment::NASC_ENV_Prod;
	NascEnvironment buttonSelected = NascEnvironment::NASC_ENV_Prod;

	bool firstRunOfState = true;

	bool buttonWasPressed = false;
	bool needsReboot = false;
	bool updateChecked = false;

	char errorString[256];

	// startup checking variables
	s64 firmwareVersion;
	std::tuple<u8, u8, u8> lumaVersion;

	s64 configVersion;
	std::tuple<u8, u8> lumaConfigVersion;

	s64 lumaOptions;
	bool gamePatchingEnabled;
	bool externalFirmsAndModulesEnabled;

	PromptState prompt;
};

#define LOG_NIMBUS_ERROR(mainStruct, fmt) \
	if (mainStruct->errorString[0] == 0) {                                       \
		snprintf(mainStruct->errorString, sizeof(mainStruct->errorString), fmt); \
	}

#define LOGF_NIMBUS_ERROR(mainStruct, fmt, ...) \
	if (mainStruct->errorString[0] == 0) {                                                    \
		snprintf(mainStruct->errorString, sizeof(mainStruct->errorString), fmt, __VA_ARGS__); \
	}

#define handleResult(action, mainStruct, name) \
	rc = action;                                                                \
	if (R_FAILED(rc)) {                                                         \
		LOGF_NIMBUS_ERROR(mainStruct, "%s failed with error: %08lx", name, rc); \
		printf("%s failed with error: %08lx\n\n", name, rc);                    \
	}

// credit to the universal-team for most/all of the code past here
extern C2D_Font font;
extern C2D_TextBuf textBuf;
extern CFG_Region loadedSystemFont;

void GetStringSize(float size, float *width, float *height, const char *text);

/*
	Get String or Text Height.

	float size: The Textsize.
	std::string Text: The Text.
	C2D_font fnt: (Optional) The wanted C2D_font. Is nullptr by default.
*/

float GetStringHeight(float size, const char *text);
void DrawString(float size, u32 color, std::string text, int flags);

void DrawControls();

// this is kinda from citro2d
CFG_Region GetSystemRegion();

void DrawScamWarning();

// code thats mostly by me again
void DrawVersionString();

bool GetLumaOptionByIndex(LumaConfigBitIndex index, s64 options);
s64 GetSystemInfoField(s32 category, CFWSystemInfoField accessor);
std::tuple<u8, u8, u8> UnpackLumaVersion(s64 packed_version);
std::tuple<u8, u8> UnpackConfigVersion(s64 packed_config_version);

void drawLumaInfo(MainStruct *mainStruct);
