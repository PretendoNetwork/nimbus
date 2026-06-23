#include "httpc.hpp"

extern "C" Handle __httpc_servhandle;

Result HTTPC_Azahar_RegisterURLReplacement(const char *pattern, const char *replacement, u32 patternSize, u32 replacementSize) {
    u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=IPC_MakeHeader(0xC00, 2, 4);
	cmdbuf[1]=patternSize;
	cmdbuf[2]=replacementSize;
	cmdbuf[3]=IPC_Desc_StaticBuffer(patternSize,0);
	cmdbuf[4]=(u32)pattern;
	cmdbuf[5]=IPC_Desc_StaticBuffer(replacementSize,1);
	cmdbuf[6]=(u32)replacement;

	Result ret=0;
	if(R_FAILED(ret=svcSendSyncRequest(__httpc_servhandle)))return ret;

	return cmdbuf[1];
}

Result HTTPC_Azahar_UnregisterURLReplacement(const char *pattern, u32 patternSize) {
    u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=IPC_MakeHeader(0xC01, 1, 2);
	cmdbuf[1]=patternSize;
	cmdbuf[2]=IPC_Desc_StaticBuffer(patternSize,0);
	cmdbuf[3]=(u32)pattern;

	Result ret=0;
	if(R_FAILED(ret=svcSendSyncRequest(__httpc_servhandle)))return ret;

	return cmdbuf[1];
}
