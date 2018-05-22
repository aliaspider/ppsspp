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

#include "GPU/GX2/VertexShaderGeneratorGX2.h"
#include "GPU/GX2/ShaderManagerGX2.h"

#include <wiiu/gx2/shader_emitter.hpp>
#include <wiiu/gx2/shader_info.h>

#include "GPU/Vulkan/VertexShaderGeneratorVulkan.h"
#include <wiiu/os/debug.h>

using namespace GX2Gen;
void GenerateVertexShaderGX2(const VShaderID &id, GX2VertexShader *vs) {
#if 1
	if (id.Bit(VS_BIT_USE_HW_TRANSFORM)) {
		*vs = TAL_VShaderGX2;
	} else {
		*vs = ST_VShaderGX2;
	}
#else
	GX2VertexShaderEmitter emit_(vs);

	Reg pos = emit_.allocReg(VSInput::POSITION);
	Reg tmp = emit_.allocReg();
	GX2Emitter::Constant proj = emit_.makeConstant(UB_Bindings::Base, offsetof(UB_VS_FS_Base, proj));

	if (id.Bit(VS_BIT_IS_THROUGH))
		proj = emit_.makeConstant(UB_Bindings::Base, offsetof(UB_VS_FS_Base, proj_through));

	emit_.MUL(___(x), pos(x), proj[0](x));
	emit_.MUL(___(y), pos(x), proj[0](y));
	emit_.MUL(___(z), pos(x), proj[0](z));
	emit_.MUL(___(w), pos(x), proj[0](w));
	emit_.ALU_LAST();
	emit_.MULADD(tmp(x), pos(y), proj[1](x), PV(x));
	emit_.MULADD(tmp(y), pos(y), proj[1](y), PV(y));
	emit_.MULADD(tmp(z), pos(y), proj[1](z), PV(z));
	emit_.MULADD(tmp(w), pos(y), proj[1](w), PV(w));
	emit_.ALU_LAST();
	emit_.MULADD(tmp(x), pos(z), proj[2](x), PV(x));
	emit_.MULADD(tmp(y), pos(z), proj[2](y), PV(y));
	emit_.MULADD(tmp(z), pos(z), proj[2](z), PV(z));
	emit_.MULADD(tmp(w), pos(z), proj[2](w), PV(w));
	emit_.ALU_LAST();
	emit_.ADD(pos(x), proj[3](x), PV(x));
	emit_.ADD(pos(y), proj[3](y), PV(y));
	emit_.ADD(pos(z), proj[3](z), PV(z));
	emit_.ADD(pos(w), proj[3](w), PV(w));
	emit_.ALU_LAST();

	emit_.EXP_DONE_POS(pos);

	if (id.Bit(VS_BIT_DO_TEXTURE)) {
		if (id.Bit(VS_BIT_HAS_COLOR))
			emit_.EXP_PARAM(PSInput::COORDS, emit_.allocReg(VSInput::COORDS), NO_BARRIER);
		else
			emit_.EXP_DONE_PARAM(PSInput::COORDS, emit_.allocReg(VSInput::COORDS), NO_BARRIER);
	}

	if (id.Bit(VS_BIT_HAS_COLOR)) {
		emit_.EXP_DONE_PARAM(PSInput::COLOR0, emit_.allocReg(VSInput::COLOR0), NO_BARRIER);
	}

	emit_.END_OF_PROGRAM();
#endif
#if 0
	DEBUG_STR(VertexShaderDesc(id).c_str());
	char buffer[16384];
	GenerateVulkanGLSLVertexShader(id, buffer);
	puts(buffer);
	GX2VertexShaderInfo(vs, buffer);
	puts(buffer);
#endif
}
