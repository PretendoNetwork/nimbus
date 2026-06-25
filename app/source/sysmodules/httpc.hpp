#pragma once

#include "../common.hpp"

Result HTTPC_Azahar_RegisterURLReplacement(const char* pattern, const char* replacement, u32 patternSize, u32 replacementSize);
Result HTTPC_Azahar_UnregisterURLReplacement(const char* pattern, u32 patternSize);