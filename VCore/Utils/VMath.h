#pragma once

#include <glm_common.h>

using v2f = glm::vec2;
using v3f = glm::vec3;
using v4f = glm::vec4;

using mtx3 = glm::mat3x3;
using mtx4 = glm::mat4x4;

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