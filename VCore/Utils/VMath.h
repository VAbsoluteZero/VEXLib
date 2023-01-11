#pragma once

#include <glm_common.h>

using v2f = glm::vec2;
using v3f = glm::vec3;
using v4f = glm::vec4;

using v2i32 = glm::i32vec2;
using v3i32 = glm::i32vec3;
using v4i32 = glm::i32vec4;

using mtx3 = glm::mat3x3;
using mtx4 = glm::mat4x4;

static inline const v2f v2f_zero = v2f{0, 0};
static inline const v2f v2f_one = v2f{1.0f, 1.0f};
static inline const v2f v2f_up = v2f{0.0f, 1.0f};
static inline const v2f v2f_left = v2f{-1.0f, 0.0f};
static inline const v2f v2f_right = v2f{1.0f, 0.0f};
static inline const v2f v2f_down = v2f{0.0f, -1.0f};

static inline const v3f v3f_zero = v3f{0.0f, 0.0f, 0.0f};
static inline const v3f v3f_one = v3f{1.0f, 1.0f, 1.0f};
static inline const v3f v3f_forward = v3f{0.0f, 1.0f, 0.0f};
static inline const v3f v3f_back = v3f{0.0f, -1.0f, 0.0f};

static inline const v2i32 v2i32_zero = v2i32{0, 0};
static inline const v2i32 v2i32_one = v2i32{1, 1};
static inline const v2i32 v2i32_up = v2i32{0, 1};
static inline const v2i32 v2i32_left = v2i32{-1, 0};
static inline const v2i32 v2i32_right = v2i32{1, 0};
static inline const v2i32 v2i32_down = v2i32{0, -1};


namespace vex
{
    static constexpr float pi(3.14159265358979323f);
    static constexpr float epsilon(1.e-8f);
    static constexpr float inaccurate_epsilon(1.e-4f);
    static constexpr float float_near_inf(3.4e+38f);

    static constexpr double double_pi(3.141592653589793238462643383279502884197169399);
    static constexpr double double_epsilon(1.e-8);
    static constexpr double double_inaccurate_epsilon(1.e-4);
    static constexpr double double_near_inf(3.4e+38);

    static constexpr float eulers(2.71828182845904523536f);
    static constexpr double double_eulers(2.7182818284590452353602874713526624977572);

    // so there is no need to include giant <algorithm> just for max
    template <typename T>
    constexpr T max(T a, T b)
    {
        return a > b ? a : b;
    }
    template <typename T>
    constexpr T min(T a, T b)
    {
        return a > b ? b : a;
    }
    // template<> // it is not actually any faster than {cond ? x : y} on pc
    // constexpr int max(int a, int b)
    //{
    //	int s = (a > b);
    //	return s * a + !s * b;
    // }

    constexpr float clampOne(float v)
    {
        v = v < 0 ? 0 : v;
        return v > 1 ? 1 : v;
    }

    inline float nearEqual(float val1, float val2, float eps = epsilon) //
    {
        return fabsf(val1 - val2) <= epsilon;
    }
} // namespace vex