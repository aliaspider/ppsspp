// Copyright (c) 2017- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#include "GPU/Common/ShaderCommon.h"
#include "GPU/Common/ShaderUniforms.h"
#include "GPU/ge_constants.h"

#include "GPU/GX2/FragmentShaderGeneratorGX2.h"
#include "GPU/GX2/ShaderManagerGX2.h"

#include <wiiu/gx2/shader_emitter.hpp>
#include <wiiu/gx2/shader_info.h>

#include "GPU/Vulkan/FragmentShaderGeneratorVulkan.h"
#include <wiiu/os/debug.h>

using namespace GX2Gen;
void GenerateFragmentShaderGX2(const FShaderID &id, GX2PixelShader *ps) {
#if 1
	*ps = PUberShaderGX2;
#else
	GX2PixelShaderEmitter emit_(ps);

	Reg color = emit_.allocReg(PSInput::COLOR0);

	if (!id.Bit(FS_BIT_CLEARMODE)) {
		if (id.Bit(FS_BIT_DO_TEXTURE)) {
			// id.Bit(FS_BIT_BGRA_TEXTURE) needed ?
			Reg coords = emit_.allocReg(PSInput::COORDS);
			Reg sample = coords;
			emit_.SAMPLE(sample, coords(x, y, _0_, _0_), 0, 0);
			switch (id.Bits(FS_BIT_TEXFUNC, 3)) {
			case GE_TEXFUNC_REPLACE:
				emit_.MOV(color(r), sample(r));
				emit_.MOV(color(g), sample(g));
				emit_.MOV(color(b), sample(b));
				emit_.ALU_LAST();
				break;
			case GE_TEXFUNC_DECAL:
				// TODO
				emit_.ADD(color(r), color(r), sample(r));
				emit_.ADD(color(g), color(g), sample(g));
				emit_.ADD(color(b), color(b), sample(b));
				emit_.ALU_LAST();
				break;
			case GE_TEXFUNC_MODULATE:
				// TODO
				emit_.ADD(color(r), color(r), sample(r));
				emit_.ADD(color(g), color(g), sample(g));
				emit_.ADD(color(b), color(b), sample(b));
				emit_.ALU_LAST();
				break;
			default:
			case GE_TEXFUNC_ADD:
				emit_.ADD(color(r), color(r), sample(r));
				emit_.ADD(color(g), color(g), sample(g));
				emit_.ADD(color(b), color(b), sample(b));
				emit_.ALU_LAST();
				break;
			}
		}
	}

	emit_.EXP_DONE_PIX(color);
	emit_.END_OF_PROGRAM();
#endif
#if 0
	DEBUG_STR(FragmentShaderDesc(id).c_str());
	char buffer[16384];
	GenerateVulkanGLSLFragmentShader(id, buffer);
	puts(buffer);
	GX2PixelShaderInfo(ps, buffer);
	puts(buffer);
#endif
}
