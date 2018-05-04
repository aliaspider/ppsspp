#pragma once
#include <wiiu/types.h>
#include "time.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   uint32_t clockSpeed;
   uint32_t __unknown0;
   OSTime baseTime;
   uint32_t __unknown[4];
}OSSystemInfo;

OSSystemInfo *OSGetSystemInfo();
u64 OSGetTitleID();
s32 * __gh_errno_ptr(void);

static inline bool OSIsHLE() { return OSGetTitleID() == 0x00050000BADF000D; }

#ifdef __cplusplus
}
#endif
