#define GX2_DISABLE_WRAPS

#include <math.h>
#include <string.h>
#include <wiiu/os/debug.h>
#include <wiiu/gx2/validation_layer.h>
#include <wiiu/gx2.h>

#define GX2_SET_FAILED(...) \
	do { \
		failed = true; \
		printf(__VA_ARGS__); \
		puts(" : "); \
	} while (0)

#define GX2_CALL_CHECKED(fn, fmt, ...) \
	do { \
		if (dump_calls || failed) \
			printf(#fn "(" fmt ")@%s:%i:%s()\n", __VA_ARGS__, remove_path(file), line, function); \
		if (failed) \
			DEBUG_CRASH(); \
		return fn(__VA_ARGS__); \
	} while (0)

static inline bool is_aligned(const void *ptr, u32 align) { return !((u32)ptr & ~(align - 1)); }

static inline bool is_in_range(const void *ptr, int size, u32 start, u32 end) { return (u32)ptr > start && (u32)ptr < end && (u32)ptr + size > start && (u32)ptr + size < end; }

static inline bool is_MEM1(const void *ptr, int size) { return is_in_range(ptr, size, 0xf4000000, 0xf6000000); }
static inline bool is_MEM2_text(const void *ptr, int size) { return is_in_range(ptr, size, 0x0C000000 /* estimate */, 0x10000000); }
static inline bool is_MEM2_data(const void *ptr, int size) { return is_in_range(ptr, size, 0x10000000, 0x50000000); }
static inline bool is_MEM2(const void *ptr, int size) { return is_MEM2_text(ptr, size) && is_MEM2_data(ptr, size); }
static inline bool is_Bucket(const void *ptr, int size) { return is_in_range(ptr, size, 0xe0000000, 0xe2800000); }
static inline bool is_valid_ptr(const void *ptr, int size) { return is_MEM1(ptr, size) || is_MEM2(ptr, size) || is_Bucket(ptr, size); }
static inline bool is_valid_data_ptr(const void *ptr, int size) { return is_MEM1(ptr, size) || is_MEM2_data(ptr, size) || is_Bucket(ptr, size); }

static inline const char *remove_path(const char *str) {
	const char *slash = strrchr(str, '/');
	return slash ? (slash + 1) : str;
}

// static bool dump_calls = true;
static bool dump_calls = false;

// static bool dump_data = true;
static bool dump_data = false;

static bool failed = false;

static inline void dump_floats(const float *ptr, int count, int stride) {
	stride /= sizeof(float);

	if (count > 4)
		count = 4;
	if (count < 1)
		count = 1;

	for (int i = 0; i < count; i++) {
		for (int j = 0; j < stride; j++) {
			if(j == 0)
				printf("(");
			else
				printf(", ");

			if(fabsf(*ptr) < 100000.0f)
				printf("%f", *ptr++);
			else
				printf("0x%08X", *(u32*)ptr++);
		}
		printf(")\n");
	}
}
static inline void dump_ints(const int *ptr, int count, int stride) {
	stride /= sizeof(int);
	if (count > 2)
		count = 2;
	if (count < 1)
		count = 1;

	for (int i = 0; i < count; i++) {
		printf("(%i", *ptr++);
		for (int j = 1; j < stride; j++) {
			printf(", %i", *ptr++);
		}
		printf(")\n");
	}
}
static inline void dump_shorts(const short *ptr, int count, int stride) {
	stride /= sizeof(short);
	count /= stride;

	if (count > 2)
		count = 2;
	if (count < 1)
		count = 1;

	for (int i = 0; i < count; i++) {
		printf("(%i", *ptr++);
		for (int j = 1; j < stride; j++) {
			printf(", %i", *ptr++);
		}
		printf(")\n");
	}
}

void GX2Invalidate_wrap(GX2InvalidateMode mode, void *buffer, uint32_t size, const char *function, int line, const char *file) {
	if (!is_MEM1(buffer, size) && !is_MEM2_data(buffer, size) && !is_Bucket(buffer, size))
		GX2_SET_FAILED("invalid buffer");

	//	if (mode == 65 && size == 12352)
	//		GX2_SET_FAILED("custom");

	GX2_CALL_CHECKED(GX2Invalidate, "%i, 0x%08X, %i", mode, buffer, size);
}

#define VBUFFERS_MAX 16
static struct {
	const void *ptr;
	u32 size;
	u32 stride;
} v_buffers[VBUFFERS_MAX];

static struct {
	int left;
	int top;
	int width;
	int height;
} scissor;

static GX2ColorBuffer color_buffer[8];
static GX2DepthBuffer depth_buffer;

void GX2SetViewport_wrap(float x, float y, float width, float height, float nearZ, float farZ, const char *function, int line, const char *file) { GX2_CALL_CHECKED(GX2SetViewport, "%f,%f,%f,%f,%f,%f", x, y, width, height, nearZ, farZ); }
void GX2SetViewportReg_wrap(GX2ViewportReg *reg, const char *function, int line, const char *file) { GX2_CALL_CHECKED(GX2SetViewportReg, "0x%08X", reg); }
void GX2SetScissor_wrap(u32 x, u32 y, u32 width, u32 height, const char *function, int line, const char *file) {
	scissor.left = x;
	scissor.top = y;
	scissor.width = width;
	scissor.height = height;
	GX2_CALL_CHECKED(GX2SetScissor, "%i,%i,%i,%i", x, y, width, height);
}
void GX2SetScissorReg_wrap(GX2ScissorReg *reg, const char *function, int line, const char *file) {
	GX2GetScissorReg(reg, (u32 *)&scissor.left, (u32 *)&scissor.top, (u32 *)&scissor.width, (u32 *)&scissor.height);
	if (dump_calls || failed)
		printf("(%i,%i,%i,%i)", scissor.left, scissor.top, scissor.width, scissor.height);

	GX2_CALL_CHECKED(GX2SetScissorReg, "0x%08X", reg);
}

void GX2SetColorBuffer_wrap(GX2ColorBuffer *colorBuffer, GX2RenderTarget target, const char *function, int line, const char *file) {
	color_buffer[target] = *colorBuffer;
	GX2_CALL_CHECKED(GX2SetColorBuffer, "0x%08X, %u", colorBuffer, target);
}
void GX2SetDepthBuffer_wrap(GX2DepthBuffer *depthBuffer, const char *function, int line, const char *file) {
	depth_buffer = *depthBuffer;
	GX2_CALL_CHECKED(GX2SetDepthBuffer, "0x%08X", depthBuffer);
}

#define GX2_CHECK(cond) \
	do { \
		if (!error && (cond)) \
			error = #cond; \
	} while (0)
const char *check_context() {
	const char* error = NULL;
	GX2_CHECK(!color_buffer[0].surface.image);

	bool depth_enabled = depth_buffer.surface.image; // TODO
	if (depth_enabled) {
		GX2_CHECK(!depth_buffer.surface.image);
		GX2_CHECK(depth_buffer.surface.width != color_buffer[0].surface.width);
		GX2_CHECK(depth_buffer.surface.height != color_buffer[0].surface.height);
	}

	GX2_CHECK(scissor.left < 0);
	GX2_CHECK(scissor.top < 0);
	GX2_CHECK(color_buffer[0].surface.width < scissor.left + scissor.width);
	GX2_CHECK(color_buffer[0].surface.height < scissor.top + scissor.height);

	if(error)
	{
		DEBUG_VAR(color_buffer[0].surface.image);
		DEBUG_INT(color_buffer[0].surface.width);
		DEBUG_INT(color_buffer[0].surface.height);
		DEBUG_VAR(depth_buffer.surface.image);
		DEBUG_INT(depth_buffer.surface.width);
		DEBUG_INT(depth_buffer.surface.height);
		DEBUG_INT(scissor.left);
		DEBUG_INT(scissor.top);
		DEBUG_INT(scissor.width);
		DEBUG_INT(scissor.height);
	}
	return error;
}
void GX2SetAttribBuffer_wrap(uint32_t index, uint32_t size, uint32_t stride, const void *buffer, const char *function, int line, const char *file) {
	if (!is_valid_data_ptr(buffer, size))
		GX2_SET_FAILED("invalid buffer");

	//	if (!is_aligned(buffer, 16))
	//		GX2_SET_FAILED("not aligned");

	if (index >= VBUFFERS_MAX)
		GX2_SET_FAILED("wrong index");

	v_buffers[index].ptr = buffer;
	v_buffers[index].size = size;
	v_buffers[index].stride = stride;

	GX2_CALL_CHECKED(GX2SetAttribBuffer, "%u, %i, %u, 0x%08X", index, size, stride, buffer);
}

void GX2DrawEx_wrap(GX2PrimitiveMode mode, uint32_t count, uint32_t offset, uint32_t numInstances, const char *function, int line, const char *file) {
	// check offset and offset + count.

	const char *error = check_context();
	if (error)
		GX2_SET_FAILED(error);

	if (dump_data || failed) {
		dump_floats(v_buffers[0].ptr, count, v_buffers[0].stride);
	}

	GX2_CALL_CHECKED(GX2DrawEx, "%i, %u, %u, %u", mode, count, offset, numInstances);
//	GX2DrawDone();
}

void GX2DrawIndexedEx_wrap(GX2PrimitiveMode mode, uint32_t count, GX2IndexType indexType, void *indices, uint32_t offset, uint32_t numInstances, const char *function, int line, const char *file) {
	u32 size = count << (1 + (indexType & 1));

	if (!is_MEM1(indices, size) && !is_MEM2_data(indices, size) && !is_Bucket(indices, size))
		GX2_SET_FAILED("invalid buffer");

	const char *error = check_context();
	if (error)
		GX2_SET_FAILED(error);

	// check offset and offset + count.

	if (dump_data || failed) {
		dump_floats(v_buffers[0].ptr, 4, v_buffers[0].stride);
		int display_count = count;
		if (display_count > 24)
			display_count = 24;
		if (indexType & 1) {
			dump_ints(indices, 1, display_count * sizeof(int));
		} else {
			dump_shorts(indices, 1, display_count * sizeof(short));
		}
	}

	GX2_CALL_CHECKED(GX2DrawIndexedEx, "%i, %u, %i, 0x%08X, %u, %u", mode, count, indexType, indices, offset, numInstances);
//	GX2DrawDone();
}
