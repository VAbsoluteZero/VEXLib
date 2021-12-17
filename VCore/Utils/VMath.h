#pragma once

#include <glm_common.h>

using v2f = glm::vec2;
using v3f = glm::vec3;
using v4f = glm::vec4;

using mtx3 = glm::mat3x3;
using mtx4 = glm::mat4x4;

static inline const v2f v2f_zero = v2f{0, 0};
static inline const v2f v2f_one = v2f{1.0f, 1.0f};
static inline const v2f v2f_up = v2f{0.0f, 1.0f};
static inline const v2f v2f_left = v2f{-1.0f, 0.0f};
static inline const v2f v2f_right = v2f{1.0f, 0.0f};
static inline const v2f v2f_down = v2f{0.0f, -1.0f};

static inline const v3f v3f_zero = v3f {0.0f, 0.0f, 0.0f};
static inline const v3f v3f_one = v3f {1.0f, 1.0f, 1.0f};
static inline const v3f v3f_forward = v3f{0.0f, 1.0f, 0.0f};
static inline const v3f v3f_back= v3f{0.0f, -1.0f, 0.0f};
// #todo add missing ones

namespace vex
{
	// so there is no need to include giant <algorithm> just for max
	template<typename T>
	constexpr T Max(T a, T b)
	{ 
		return a > b ? a : b;
	}
	template<typename T>
	constexpr T Min(T a, T b)
	{
		return a > b ? b : a;
	}
	template<> // it is not actually any faster than {cond ? x : y} on pc
	constexpr int Max(int a, int b)
	{
		int s = (a > b);
		return s * a + !s * b;
	}

	constexpr float ClampOne(float v)
	{
		v = v < 0 ? 0 : v;
		return v > 1 ? 1 : v;
	}
}