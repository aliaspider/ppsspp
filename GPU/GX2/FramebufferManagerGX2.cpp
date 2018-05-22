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

#include "profiler/profiler.h"
#include "base/display.h"
#include "math/lin/matrix4x4.h"
#include "ext/native/thin3d/thin3d.h"
#include "base/basictypes.h"
#include "file/vfs.h"
#include "file/zip_read.h"
#include "i18n/i18n.h"

#include "Common/ColorConv.h"
#include "Common/MathUtil.h"
#include "Core/Host.h"
#include "Core/MemMap.h"
#include "Core/Config.h"
#include "Core/System.h"
#include "Core/Reporting.h"
#include "GPU/ge_constants.h"
#include "GPU/GPUState.h"
#include "GPU/Debugger/Stepping.h"

#include "GPU/Common/FramebufferCommon.h"
#include "GPU/Common/ShaderTranslation.h"
#include "GPU/Common/TextureDecoder.h"
#include "GPU/Common/PostShader.h"
#include "GPU/GX2/FramebufferManagerGX2.h"
#include "GPU/GX2/ShaderManagerGX2.h"
#include "GPU/GX2/TextureCacheGX2.h"
#include "GPU/GX2/DrawEngineGX2.h"
#include "GPU/GX2/GX2Shaders.h"

#include "ext/native/thin3d/thin3d.h"

#include <wiiu/gx2.h>
#include <algorithm>

// clang-format off
const GX2AttribStream FramebufferManagerGX2::g_QuadAttribStream[2] = {
	{ 0, 0, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32, GX2_ATTRIB_INDEX_PER_VERTEX, 0, GX2_COMP_SEL(_x, _y, _z, _1), GX2_ENDIAN_SWAP_DEFAULT },
	{ 1, 0, 12, GX2_ATTRIB_FORMAT_FLOAT_32_32, GX2_ATTRIB_INDEX_PER_VERTEX, 0, GX2_COMP_SEL(_x, _y, _0, _0), GX2_ENDIAN_SWAP_DEFAULT },
};

// STRIP geometry
__attribute__((aligned(GX2_VERTEX_BUFFER_ALIGNMENT)))
float FramebufferManagerGX2::fsQuadBuffer_[20] = {
	-1.0f,-1.0f, 0.0f, 0.0f, 0.0f,
	 1.0f,-1.0f, 0.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
};
// clang-format on

FramebufferManagerGX2::FramebufferManagerGX2(Draw::DrawContext *draw) : FramebufferManagerCommon(draw) {
	context_ = (GX2ContextState *)draw->GetNativeObject(Draw::NativeObject::CONTEXT);

	quadFetchShader_.size = GX2CalcFetchShaderSize(ARRAY_SIZE(g_QuadAttribStream));
	quadFetchShader_.program = (u8 *)MEM2_alloc(quadFetchShader_.size, GX2_SHADER_ALIGNMENT);
	GX2InitFetchShader(&quadFetchShader_, quadFetchShader_.program, ARRAY_SIZE(g_QuadAttribStream), g_QuadAttribStream);
	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, quadFetchShader_.program, quadFetchShader_.size);

	quadBuffer_ = (float*)MEM2_alloc(quadStride_ * 4, GX2_VERTEX_BUFFER_ALIGNMENT);
	postConstants_ = MEM2_alloc(ROUND_UP(sizeof(PostShaderUniforms), 64), GX2_UNIFORM_BLOCK_ALIGNMENT);

	for (int i = 0; i < 256; i++) {
		GX2InitStencilMaskReg(&stencilMaskStates_[i], i, 0xFF, 0xFF, i, 0xFF, 0xFF);
	}

	ShaderTranslationInit();
	CompilePostShader();
}

FramebufferManagerGX2::~FramebufferManagerGX2() {
	ShaderTranslationShutdown();

	// Drawing cleanup
	MEM2_free(quadFetchShader_.program);
	MEM2_free(quadBuffer_);
	MEM2_free(postConstants_);

	if (drawPixelsTex_.surface.image)
		MEM2_free(drawPixelsTex_.surface.image);

	if (postFetchShader_.program) {
		MEM2_free(postFetchShader_.program);
	}

	// FBO cleanup
	for (auto it = tempFBOs_.begin(), end = tempFBOs_.end(); it != end; ++it) {
		it->second.fbo->Release();
	}

	// Stencil cleanup
	if (stencilValueBuffer_)
		MEM2_free(stencilValueBuffer_);
}

void FramebufferManagerGX2::SetTextureCache(TextureCacheGX2 *tc) {
	textureCacheGX2_ = tc;
	textureCache_ = tc;
}

void FramebufferManagerGX2::SetShaderManager(ShaderManagerGX2 *sm) {
	shaderManagerGX2_ = sm;
	shaderManager_ = sm;
}

void FramebufferManagerGX2::SetDrawEngine(DrawEngineGX2 *td) {
	drawEngineGX2_ = td;
	drawEngine_ = td;
}

void FramebufferManagerGX2::CompilePostShader() {
	// TODO
}

void FramebufferManagerGX2::MakePixelTexture(const u8 *srcPixels, GEBufferFormat srcPixelFormat, int srcStride, int width, int height, float &u1, float &v1) {
	PROFILE_THIS_SCOPE("make pixtex");
	if (drawPixelsTex_.surface.image && (drawPixelsTex_.surface.width != width || drawPixelsTex_.surface.height != height)) {
		MEM2_free(drawPixelsTex_.surface.image);
		drawPixelsTex_ = {};
	}

	if (!drawPixelsTex_.surface.image) {
		drawPixelsTex_.surface.width = width;
		drawPixelsTex_.surface.height = height;
		drawPixelsTex_.surface.depth = 1;
		drawPixelsTex_.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
		drawPixelsTex_.surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
		drawPixelsTex_.surface.use = GX2_SURFACE_USE_TEXTURE;
		drawPixelsTex_.viewNumSlices = 1;

		drawPixelsTex_.surface.format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
		drawPixelsTex_.compMap = GX2_COMP_SEL(_r, _g, _b, _a);

		GX2CalcSurfaceSizeAndAlignment(&drawPixelsTex_.surface);
		GX2InitTextureRegs(&drawPixelsTex_);

		drawPixelsTex_.surface.image = MEM2_alloc(drawPixelsTex_.surface.imageSize, drawPixelsTex_.surface.alignment);
		_assert_(drawPixelsTex_.surface.image);
	}

	u32_le *dst = (u32_le *)drawPixelsTex_.surface.image;
	for (int y = 0; y < height; y++) {
		if (srcPixelFormat != GE_FORMAT_8888) {
			const u16_le *src = (const u16_le *)srcPixels + srcStride * y;
			for (u32 x = 0; x < width; x++) {
				switch (srcPixelFormat) {
				case GE_FORMAT_565: dst[x] = RGB565ToRGBA8888(src[x]); break;
				case GE_FORMAT_5551: dst[x] = RGBA5551ToRGBA8888(src[x]); break;
				case GE_FORMAT_4444: dst[x] = RGBA4444ToRGBA8888(src[x]); break;
				}
			}
		} else {
			const u32_le *src = (const u32_le *)srcPixels + srcStride * y;
			memcpy(dst, src, width * sizeof(u32_le));
		}
		dst += drawPixelsTex_.surface.pitch;
	}
	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, drawPixelsTex_.surface.image, drawPixelsTex_.surface.imageSize);
	GX2SetPixelTexture(&drawPixelsTex_, 0);
}

void FramebufferManagerGX2::DrawActiveTexture(float x, float y, float w, float h, float destW, float destH, float u0, float v0, float u1, float v1, int uvRotation, int flags) {
	struct Coord {
		Vec3 pos;
		float u, v;
	};
	Coord coord[4] = {
		{ { x, y, 0 }, u0, v0 },
		{ { x + w, y, 0 }, u1, v0 },
		{ { x + w, y + h, 0 }, u1, v1 },
		{ { x, y + h, 0 }, u0, v1 },
	};

	if (uvRotation != ROTATION_LOCKED_HORIZONTAL) {
		float temp[8];
		int rotation = 0;
		switch (uvRotation) {
		case ROTATION_LOCKED_HORIZONTAL180: rotation = 2; break;
		case ROTATION_LOCKED_VERTICAL: rotation = 1; break;
		case ROTATION_LOCKED_VERTICAL180: rotation = 3; break;
		}
		for (int i = 0; i < 4; i++) {
			temp[i * 2] = coord[((i + rotation) & 3)].u;
			temp[i * 2 + 1] = coord[((i + rotation) & 3)].v;
		}

		for (int i = 0; i < 4; i++) {
			coord[i].u = temp[i * 2];
			coord[i].v = temp[i * 2 + 1];
		}
	}

	float invDestW = 1.0f / (destW * 0.5f);
	float invDestH = 1.0f / (destH * 0.5f);
	for (int i = 0; i < 4; i++) {
		coord[i].pos.x = coord[i].pos.x * invDestW - 1.0f;
		coord[i].pos.y = -(coord[i].pos.y * invDestH - 1.0f);
	}

	if (g_display_rotation != DisplayRotation::ROTATE_0) {
		for (int i = 0; i < 4; i++) {
			// backwards notation, should fix that...
			coord[i].pos = coord[i].pos * g_display_rot_matrix;
		}
	}

	// The above code is for FAN geometry but we can only do STRIP. So rearrange it a little.
	memcpy(quadBuffer_, coord, sizeof(Coord));
	memcpy(quadBuffer_ + 5, coord + 1, sizeof(Coord));
	memcpy(quadBuffer_ + 10, coord + 3, sizeof(Coord));
	memcpy(quadBuffer_ + 15, coord + 2, sizeof(Coord));
	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, quadBuffer_, sizeof(coord));

	GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, GX2_DISABLE, GX2_DISABLE);
	GX2SetColorControlReg(&StockGX2::blendDisabledColorWrite);
	GX2SetTargetChannelMasksReg(&StockGX2::TargetChannelMasks[0xF]);
	GX2SetDepthStencilControlReg(&StockGX2::depthStencilDisabled);
	GX2SetPixelSampler((flags & DRAWTEX_LINEAR) ? &StockGX2::samplerLinear2DClamp : &StockGX2::samplerPoint2DClamp, 0);
	GX2SetAttribBuffer(0, sizeof(coord), sizeof(*coord), quadBuffer_);
	GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLE_STRIP, 4, 0, 1);
	gstate_c.Dirty(DIRTY_BLEND_STATE | DIRTY_DEPTHSTENCIL_STATE | DIRTY_RASTER_STATE);
}

void FramebufferManagerGX2::Bind2DShader() {
	GX2SetFetchShader(&quadFetchShader_);
	GX2SetPixelShader(&defPShaderGX2);
	GX2SetVertexShader(&defVShaderGX2);
}

void FramebufferManagerGX2::BindPostShader(const PostShaderUniforms &uniforms) {
	if (!postPixelShader_) {
		if (usePostShader_) {
			CompilePostShader();
		}
		if (!usePostShader_) {
			SetNumExtraFBOs(0);
			GX2SetFetchShader(&quadFetchShader_);
			GX2SetPixelShader(&defPShaderGX2);
			GX2SetVertexShader(&defVShaderGX2);
			return;
		} else {
			SetNumExtraFBOs(1);
		}
	}
	GX2SetFetchShader(&postFetchShader_);
	GX2SetPixelShader(postPixelShader_);
	GX2SetVertexShader(postVertexShader_);

	memcpy(postConstants_, &uniforms, sizeof(uniforms));
	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_UNIFORM_BLOCK, postConstants_, sizeof(uniforms));
	GX2SetVertexUniformBlock(1, sizeof(uniforms), postConstants_); // Probably not necessary
	GX2SetPixelUniformBlock(1, sizeof(uniforms), postConstants_);
}

void FramebufferManagerGX2::ReformatFramebufferFrom(VirtualFramebuffer *vfb, GEBufferFormat old) {
	if (!useBufferedRendering_ || !vfb->fbo) {
		return;
	}

	// Technically, we should at this point re-interpret the bytes of the old format to the new.
	// That might get tricky, and could cause unnecessary slowness in some games.
	// For now, we just clear alpha/stencil from 565, which fixes shadow issues in Kingdom Hearts.
	// (it uses 565 to write zeros to the buffer, than 4444 to actually render the shadow.)
	//
	// The best way to do this may ultimately be to create a new FBO (combine with any resize?)
	// and blit with a shader to that, then replace the FBO on vfb.  Stencil would still be complex
	// to exactly reproduce in 4444 and 8888 formats.
	if (old == GE_FORMAT_565) {
		draw_->BindFramebufferAsRenderTarget(vfb->fbo, { Draw::RPAction::CLEAR, Draw::RPAction::KEEP, Draw::RPAction::KEEP });

		// TODO: There's no way this does anything useful :(
		GX2SetDepthStencilControlReg(&StockGX2::depthDisabledStencilWrite);
		GX2SetStencilMask(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF); // TODO, and maybe GX2SetStencilMaskReg?
		GX2SetColorControlReg(&StockGX2::blendColorDisabled);
		GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, GX2_DISABLE, GX2_DISABLE);
		GX2SetFetchShader(&quadFetchShader_);
		GX2SetPixelShader(&defPShaderGX2);
		GX2SetVertexShader(&defVShaderGX2);
		GX2SetAttribBuffer(0, sizeof(fsQuadBuffer_), quadStride_, fsQuadBuffer_);
		shaderManagerGX2_->DirtyLastShader();
		GX2SetViewport( 0.0f, 0.0f, (float)vfb->renderWidth, (float)vfb->renderHeight, 0.0f, 1.0f);
		GX2SetScissor(0, 0, vfb->renderWidth, vfb->renderHeight);
		GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLE_STRIP, 4, 0, 1);
	}

	RebindFramebuffer();
	gstate_c.Dirty(DIRTY_BLEND_STATE | DIRTY_DEPTHSTENCIL_STATE | DIRTY_RASTER_STATE | DIRTY_VIEWPORTSCISSOR_STATE | DIRTY_VERTEXSHADER_STATE);
}

static void CopyPixelDepthOnly(u32 *dstp, const u32 *srcp, size_t c) {
	for (size_t x = 0; x < c; ++x) {
		memcpy(dstp + x, srcp + x, 3);
	}
}

void FramebufferManagerGX2::BlitFramebufferDepth(VirtualFramebuffer *src, VirtualFramebuffer *dst) {
	if (g_Config.bDisableSlowFramebufEffects) {
		return;
	}
	bool matchingDepthBuffer = src->z_address == dst->z_address && src->z_stride != 0 && dst->z_stride != 0;
	bool matchingSize = src->width == dst->width && src->height == dst->height;
	bool matchingRenderSize = src->renderWidth == dst->renderWidth && src->renderHeight == dst->renderHeight;
	if (matchingDepthBuffer && matchingSize && matchingRenderSize) {
		// TODO: Currently, this copies depth AND stencil, which is a problem.  See #9740.
		draw_->CopyFramebufferImage(src->fbo, 0, 0, 0, 0, dst->fbo, 0, 0, 0, 0, src->renderWidth, src->renderHeight, 1, Draw::FB_DEPTH_BIT);
		RebindFramebuffer();
		dst->last_frame_depth_updated = gpuStats.numFlips;
	}
}

void FramebufferManagerGX2::BindFramebufferAsColorTexture(int stage, VirtualFramebuffer *framebuffer, int flags) {
	if (!framebuffer->fbo || !useBufferedRendering_) {
		// GX2SetPixelTexture(nullptr, 1); // TODO: what is the correct way to unbind a texture ?
		gstate_c.skipDrawReason |= SKIPDRAW_BAD_FB_TEXTURE;
		return;
	}

	// currentRenderVfb_ will always be set when this is called, except from the GE debugger.
	// Let's just not bother with the copy in that case.
	bool skipCopy = (flags & BINDFBCOLOR_MAY_COPY) == 0;
	if (GPUStepping::IsStepping() || g_Config.bDisableSlowFramebufEffects) {
		skipCopy = true;
	}
	// Currently rendering to this framebuffer. Need to make a copy.
	if (!skipCopy && framebuffer == currentRenderVfb_) {
		// TODO: Maybe merge with bvfbs_?  Not sure if those could be packing, and they're created at a different size.
		Draw::Framebuffer *renderCopy = GetTempFBO(framebuffer->renderWidth, framebuffer->renderHeight, (Draw::FBColorDepth)framebuffer->colorDepth);
		if (renderCopy) {
			VirtualFramebuffer copyInfo = *framebuffer;
			copyInfo.fbo = renderCopy;
			CopyFramebufferForColorTexture(&copyInfo, framebuffer, flags);
			RebindFramebuffer();
			draw_->BindFramebufferAsTexture(renderCopy, stage, Draw::FB_COLOR_BIT, 0);
		} else {
			draw_->BindFramebufferAsTexture(framebuffer->fbo, stage, Draw::FB_COLOR_BIT, 0);
		}
	} else if (framebuffer != currentRenderVfb_) {
		draw_->BindFramebufferAsTexture(framebuffer->fbo, stage, Draw::FB_COLOR_BIT, 0);
	} else {
		ERROR_LOG_REPORT_ONCE(GX2SelfTexture, G3D, "Attempting to texture from target (src=%08x / target=%08x / flags=%d)", framebuffer->fb_address, currentRenderVfb_->fb_address, flags);
		// Badness on GX2 to bind the currently rendered-to framebuffer as a texture.
		// GX2SetPixelTexture(nullptr, 1); // TODO: what is the correct way to unbind a texture ?
		gstate_c.skipDrawReason |= SKIPDRAW_BAD_FB_TEXTURE;
		return;
	}
}

bool FramebufferManagerGX2::CreateDownloadTempBuffer(VirtualFramebuffer *nvfb) {
	nvfb->colorDepth = Draw::FBO_8888;

	nvfb->fbo = draw_->CreateFramebuffer({ nvfb->width, nvfb->height, 1, 1, true, (Draw::FBColorDepth)nvfb->colorDepth });
	if (!(nvfb->fbo)) {
		ERROR_LOG(FRAMEBUF, "Error creating FBO! %i x %i", nvfb->renderWidth, nvfb->renderHeight);
		return false;
	}

	draw_->BindFramebufferAsRenderTarget(nvfb->fbo, { Draw::RPAction::CLEAR, Draw::RPAction::CLEAR, Draw::RPAction::CLEAR });
	return true;
}

void FramebufferManagerGX2::UpdateDownloadTempBuffer(VirtualFramebuffer *nvfb) {
	// Nothing to do here.
}

void FramebufferManagerGX2::SimpleBlit(Draw::Framebuffer *dest, float destX1, float destY1, float destX2, float destY2, Draw::Framebuffer *src, float srcX1, float srcY1, float srcX2, float srcY2, bool linearFilter) {
	int destW, destH, srcW, srcH;
	draw_->GetFramebufferDimensions(src, &srcW, &srcH);
	draw_->GetFramebufferDimensions(dest, &destW, &destH);

	if (srcW == destW && srcH == destH && destX2 - destX1 == srcX2 - srcX1 && destY2 - destY1 == srcY2 - srcY1) {
		// Optimize to a copy
		draw_->CopyFramebufferImage(src, 0, (int)srcX1, (int)srcY1, 0, dest, 0, (int)destX1, (int)destY1, 0, (int)(srcX2 - srcX1), (int)(srcY2 - srcY1), 1, Draw::FB_COLOR_BIT);
		return;
	}

	float dX = 1.0f / (float)destW;
	float dY = 1.0f / (float)destH;
	float sX = 1.0f / (float)srcW;
	float sY = 1.0f / (float)srcH;
	struct Vtx {
		float x, y, z, u, v;
	};
	Vtx vtx[4] = {
		{ -1.0f + 2.0f * dX * destX1, 1.0f - 2.0f * dY * destY1, 0.0f, sX * srcX1, sY * srcY1 },
		{ -1.0f + 2.0f * dX * destX2, 1.0f - 2.0f * dY * destY1, 0.0f, sX * srcX2, sY * srcY1 },
		{ -1.0f + 2.0f * dX * destX1, 1.0f - 2.0f * dY * destY2, 0.0f, sX * srcX1, sY * srcY2 },
		{ -1.0f + 2.0f * dX * destX2, 1.0f - 2.0f * dY * destY2, 0.0f, sX * srcX2, sY * srcY2 },
	};

	memcpy(quadBuffer_, vtx, 4 * sizeof(Vtx));
	GX2Invalidate(GX2_INVALIDATE_MODE_ATTRIBUTE_BUFFER, quadBuffer_, 4 * sizeof(Vtx));

	// Unbind the texture first to avoid the GX2 hazard check (can't set render target to things bound as textures and vice versa, not even temporarily).
	draw_->BindTexture(0, nullptr);
	draw_->BindFramebufferAsRenderTarget(dest, { Draw::RPAction::KEEP, Draw::RPAction::KEEP, Draw::RPAction::KEEP });
	draw_->BindFramebufferAsTexture(src, 0, Draw::FB_COLOR_BIT, 0);

	Bind2DShader();
	GX2SetViewport( 0.0f, 0.0f, (float)destW, (float)destH, 0.0f, 1.0f );
	GX2SetScissor(0, 0, destW, destH);
	GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, GX2_DISABLE, GX2_DISABLE);
	GX2SetColorControlReg(&StockGX2::blendDisabledColorWrite);
	GX2SetTargetChannelMasksReg(&StockGX2::TargetChannelMasks[0xF]);
	GX2SetDepthStencilControlReg(&StockGX2::depthStencilDisabled);
	GX2SetPixelSampler(linearFilter ? &StockGX2::samplerLinear2DClamp : &StockGX2::samplerPoint2DClamp, 0);
	GX2SetAttribBuffer(0, 4 * sizeof(Vtx), sizeof(Vtx), quadBuffer_);
	GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLE_STRIP, 4, 0, 1);

	gstate_c.Dirty(DIRTY_BLEND_STATE | DIRTY_DEPTHSTENCIL_STATE | DIRTY_RASTER_STATE | DIRTY_VIEWPORTSCISSOR_STATE | DIRTY_VERTEXSHADER_STATE);
}

void FramebufferManagerGX2::BlitFramebuffer(VirtualFramebuffer *dst, int dstX, int dstY, VirtualFramebuffer *src, int srcX, int srcY, int w, int h, int bpp) {
	if (!dst->fbo || !src->fbo || !useBufferedRendering_) {
		// This can happen if they recently switched from non-buffered.
		if (useBufferedRendering_) {
			draw_->BindFramebufferAsRenderTarget(nullptr, { Draw::RPAction::KEEP, Draw::RPAction::KEEP, Draw::RPAction::KEEP });
		}
		return;
	}

	float srcXFactor = (float)src->renderWidth / (float)src->bufferWidth;
	float srcYFactor = (float)src->renderHeight / (float)src->bufferHeight;
	const int srcBpp = src->format == GE_FORMAT_8888 ? 4 : 2;
	if (srcBpp != bpp && bpp != 0) {
		srcXFactor = (srcXFactor * bpp) / srcBpp;
	}
	int srcX1 = srcX * srcXFactor;
	int srcX2 = (srcX + w) * srcXFactor;
	int srcY1 = srcY * srcYFactor;
	int srcY2 = (srcY + h) * srcYFactor;

	float dstXFactor = (float)dst->renderWidth / (float)dst->bufferWidth;
	float dstYFactor = (float)dst->renderHeight / (float)dst->bufferHeight;
	const int dstBpp = dst->format == GE_FORMAT_8888 ? 4 : 2;
	if (dstBpp != bpp && bpp != 0) {
		dstXFactor = (dstXFactor * bpp) / dstBpp;
	}
	int dstX1 = dstX * dstXFactor;
	int dstX2 = (dstX + w) * dstXFactor;
	int dstY1 = dstY * dstYFactor;
	int dstY2 = (dstY + h) * dstYFactor;

	// Direct3D doesn't support rect -> self.
	Draw::Framebuffer *srcFBO = src->fbo;
	if (src == dst) {
		Draw::Framebuffer *tempFBO = GetTempFBO(src->renderWidth, src->renderHeight, (Draw::FBColorDepth)src->colorDepth);
		SimpleBlit(tempFBO, dstX1, dstY1, dstX2, dstY2, src->fbo, srcX1, srcY1, srcX2, srcY2, false);
		srcFBO = tempFBO;
	}
	SimpleBlit(dst->fbo, dstX1, dstY1, dstX2, dstY2, srcFBO, srcX1, srcY1, srcX2, srcY2, false);
}

// Nobody calls this yet.
void FramebufferManagerGX2::PackDepthbuffer(VirtualFramebuffer *vfb, int x, int y, int w, int h) {
	if (!vfb->fbo) {
		ERROR_LOG_REPORT_ONCE(vfbfbozero, SCEGE, "PackDepthbuffer: vfb->fbo == 0");
		return;
	}

	const u32 z_address = (0x04000000) | vfb->z_address;
	// TODO
}

void FramebufferManagerGX2::EndFrame() {}

void FramebufferManagerGX2::DeviceLost() { DestroyAllFBOs(); }

void FramebufferManagerGX2::DestroyAllFBOs() {
	currentRenderVfb_ = nullptr;
	displayFramebuf_ = nullptr;
	prevDisplayFramebuf_ = nullptr;
	prevPrevDisplayFramebuf_ = nullptr;

	for (size_t i = 0; i < vfbs_.size(); ++i) {
		VirtualFramebuffer *vfb = vfbs_[i];
		INFO_LOG(FRAMEBUF, "Destroying FBO for %08x : %i x %i x %i", vfb->fb_address, vfb->width, vfb->height, vfb->format);
		DestroyFramebuf(vfb);
	}
	vfbs_.clear();

	for (size_t i = 0; i < bvfbs_.size(); ++i) {
		VirtualFramebuffer *vfb = bvfbs_[i];
		DestroyFramebuf(vfb);
	}
	bvfbs_.clear();

	for (auto it = tempFBOs_.begin(), end = tempFBOs_.end(); it != end; ++it) {
		it->second.fbo->Release();
	}
	tempFBOs_.clear();

	SetNumExtraFBOs(0);
}

void FramebufferManagerGX2::Resized() {
	FramebufferManagerCommon::Resized();

	if (UpdateSize()) {
		DestroyAllFBOs();
	}

	// Might have a new post shader - let's compile it.
	CompilePostShader();
}
