#pragma once

#define GX2_SCAN_BUFFER_ALIGNMENT      0x1000
#define GX2_SHADER_ALIGNMENT           0x100
#define GX2_CONTEXT_STATE_ALIGNMENT    0x100
#define GX2_UNIFORM_BLOCK_ALIGNMENT    0x100
#define GX2_DISPLAY_LIST_ALIGNMENT     0x20
#define GX2_VERTEX_BUFFER_ALIGNMENT    0x40
#define GX2_INDEX_BUFFER_ALIGNMENT     0x20

#define GX2_ENABLE                     TRUE
#define GX2_DISABLE                    FALSE

#define GX2_TRUE                       TRUE
#define GX2_FALSE                      FALSE

#ifndef GX2_COMP_SEL
#define GX2_COMP_SEL(c0, c1, c2, c3) (((c0) << 24) | ((c1) << 16) | ((c2) << 8) | (c3))

#define _x 0
#define _y 1
#define _z 2
#define _w 3
#define _r 0
#define _g 1
#define _b 2
#define _a 3
#define _0 4
#define _1 5
#endif
