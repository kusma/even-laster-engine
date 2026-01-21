#ifndef CORE_H
#define CORE_H

#include <stdint.h>
#include <stdfloat>

#if defined(_MSC_VER) && _MSC_VER < 1900
#define _CRT_SECURE_NO_WARNINGS
#define snprintf _snprintf
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

#ifdef _MSC_VER
#define UNREACHABLE(str) \
do { \
	assert(!str); \
	__assume(0); \
} while (0);
#else
#define UNREACHABLE(str) assert(!str)
#endif

#ifdef _MSC_VER
#include <intrin.h>
static inline uint32_t clz(uint32_t x)
{
	unsigned long r = 0;
	if (_BitScanReverse(&r, x))
		return 31 - r;
	else
		return 32;
}
#elif defined(__GNUC__)
static inline uint32_t clz(uint32_t x)
{
	return x ? __builtin_clz(x) : 32;
}
#else
static inline uint32_t clz(uint32_t x)
{
	unsigned n = 0;
	for (int i = 1; i < 32; i++) {
		if (x & (1 << 31))
			return n;
		n++;
		x <<= 1;
	}
	return n;
}
#endif

#if __STDCPP_FLOAT16_T__ == 1

inline uint16_t float_to_half(float input)
{
	union {
		uint16_t u;
		std::float16_t f;
	} u;
	u.f = std::float16_t(input);
	return u.u;
}

#elif defined(__F16C__)

#include <immintrin.h>
inline uint16_t float_to_half(float input)
{
	__m128 single = _mm_set_ss(input);
	__m128i half = _mm_cvtps_ph(single, 0);
	return static_cast<uint16_t>(_mm_cvtsi128_si32(half));
}

#elif defined(__ARM_NEON__)

#include <arm_neon.h>
inline uint16_t float_to_half(float input)
{
	float array[4] = {input, input, input, input};
	float32x4_t input4 = vld1q_f32(array);
	float16x4_t result = vcvt_f16_f32(input4);
	return vget_lane_f16(result, 0);
}

#else
#error "16-bit float type required"
#endif

#endif // CORE_H
