#include "common.hpp"
#include <3ds/services/cfgu.h>
#include <format>

// credit to the universal-team for most/all of the code past here
C2D_Font font;
C2D_TextBuf textBuf;
CFG_Region loadedSystemFont = GetSystemRegion();

void GetStringSize(float size, float *width, float *height, const char *text) {
	C2D_Text c2d_text;

	C2D_TextFontParse(&c2d_text, font, textBuf, text);

	C2D_TextGetDimensions(&c2d_text, size, size, width, height);
}

float GetStringHeight(float size, const char *text) {
	float height = 0;

	GetStringSize(size, NULL, &height, text);

	return height;
}

void DrawString(float size, u32 color, std::string text, int flags) {
	C2D_Text c2d_text;
	
	C2D_TextBufClear(textBuf);

	C2D_TextFontParse(&c2d_text, font, textBuf, text.c_str());
	C2D_TextOptimize(&c2d_text);
	
	float x = 8;
	float y = 8;
	
	float screenWidthBottom = 320;
	float screenHeight = 240;
	
	switch (loadedSystemFont) {
		case CFG_REGION_CHN:
		case CFG_REGION_KOR:
		case CFG_REGION_TWN:
			y += 3.0f * size;
			break;
		default:
			break;
	}

	float heightScale = std::min(size, size*((screenHeight - y * 2)/GetStringHeight(size, text.c_str())));

	C2D_DrawText(&c2d_text, C2D_WithColor | C2D_WordWrap | flags, x, y, 0.5f, size, heightScale, color, screenWidthBottom - x * 2);
}

void DrawControls() {
	C2D_Text c2d_text;
	const char* text = "A: Select\nY: Launch plugin\nX: Unlink PNID";
	float size = 0.5f;
	int offset = 6;
	int bottomOffset = (240 + 2)- offset; // 3ds vertical resolution - offset, but add 2 to the vertical resolution to account for the vertical padding?

	C2D_TextBufClear(textBuf);

	C2D_TextFontParse(&c2d_text, font, textBuf, text);
	C2D_TextOptimize(&c2d_text);

	C2D_DrawText(&c2d_text, C2D_WithColor | C2D_AlignLeft, offset, bottomOffset - GetStringHeight(size, text), 0.5f, size, size, C2D_Color32(255, 255, 255, 0xFF));

}

// this is kinda from citro2d
CFG_Region GetSystemRegion() {
	u8 systemRegion = 0;
	Result rc = CFGU_SecureInfoGetRegion(&systemRegion);
	if (R_FAILED(rc))
	{
		return CFG_REGION_USA;
	}
	
	return static_cast<CFG_Region>(systemRegion);
}

void DrawScamWarning() {
	C2D_Text c2d_text1;
	C2D_Text c2d_text2;
	std::string line1 = std::format("{} is free and open-source. If you paid to have it installed,", APP_TITLE);
	std::string line2 = "you have been scammed. Demand your money back";
	float size = 0.5f;
	int offset = 6;
	int horizontalOffset = offset;
	float line2Y = (240 + 2) - offset - GetStringHeight(size, line2.c_str());
	float line1Y = line2Y - GetStringHeight(size, line1.c_str());

	C2D_TextBufClear(textBuf);

	C2D_TextFontParse(&c2d_text1, font, textBuf, line1.c_str());
	C2D_TextOptimize(&c2d_text1);

	C2D_TextFontParse(&c2d_text2, font, textBuf, line2.c_str());
	C2D_TextOptimize(&c2d_text2);

	C2D_DrawText(&c2d_text1, C2D_WithColor | C2D_AlignLeft, horizontalOffset, line1Y, 0.5f, size, size, C2D_Color32(255, 255, 255, 0xFF));
	C2D_DrawText(&c2d_text2, C2D_WithColor | C2D_AlignLeft, horizontalOffset, line2Y, 0.5f, size, size, C2D_Color32(255, 255, 255, 0xFF));
}

// code thats mostly by me again 
void DrawVersionString() {
	C2D_Text c2d_text;
	std::string text = std::format("{} {}.{}.{}", APP_TITLE, VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
	float size = 0.5f;
	int offset = 6;
	int horizontalOffset = 400 - offset;

	C2D_TextBufClear(textBuf);

	C2D_TextFontParse(&c2d_text, font, textBuf, text.c_str());
	C2D_TextOptimize(&c2d_text);

	C2D_DrawText(&c2d_text, C2D_WithColor | C2D_AlignRight, horizontalOffset, offset, 0.5f, size, size, C2D_Color32(255, 255, 255, 0xFF));
}

bool GetLumaOptionByIndex(LumaConfigBitIndex index, s64 options) {
    return ((options >> (static_cast<s32>(index))) & 0x1) == 1; // if the last bit of options shifted by the right index amount of bits is 1, return true
}

s64 GetSystemInfoField(s32 category, CFWSystemInfoField accessor) {
	s64 out = 0;
	svcGetSystemInfo(&out, category, static_cast<s32>(accessor));
	
	return out;
}

std::tuple<u8, u8, u8> UnpackLumaVersion(s64 packed_version) {
	return { (packed_version >> 24) & 0xFF, (packed_version >> 16) & 0xFF, (packed_version >> 8) & 0xFF };
}

std::tuple<u8, u8> UnpackConfigVersion(s64 packed_config_version) {
	return { (packed_config_version >> 16) & 0xFF, packed_config_version & 0xFF };
}

void drawLumaInfo(MainStruct *mainStruct) {
	DrawString(0.5f, defaultColor, std::format("Luma version is {}.{}.{}\nLuma config version is {}.{}\n\nLuma3DS config bits are:\n{:016b}\n{:016b}\n{:016b}\n{:016b}",
	std::get<0>(mainStruct->lumaVersion), std::get<1>(mainStruct->lumaVersion), std::get<2>(mainStruct->lumaVersion),
	std::get<0>(mainStruct->lumaConfigVersion), std::get<1>(mainStruct->lumaConfigVersion), mainStruct->lumaOptions >> 48,
	(mainStruct->lumaOptions >> 32) & 0xFFFF, (mainStruct->lumaOptions >> 16) & 0xFFFF, mainStruct->lumaOptions & 0xFFFF), 0);
}
