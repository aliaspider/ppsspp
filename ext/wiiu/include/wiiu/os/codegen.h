#pragma once
#include <wiiu/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void OSGetCodegenVirtAddrRange(void **outaddr, u32 *outsize);
u32 OSGetCodegenCore(void);
u32 OSGetCodegenMode(void);
u32 OSSwitchSecCodeGenMode(bool exec);
u32 OSGetSecCodeGenMode(void);
u32 OSCodegenCopy(void *dstaddr, void *srcaddr, u32 size);

#ifdef __cplusplus
}
#endif
