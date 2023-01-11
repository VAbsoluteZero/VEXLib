#pragma once

#include <glm_common.h>

using v2f = glm::vec2;
using v3f = glm::vec3;
using v4f = glm::vec4;

using v2i32 = glm::i32vec2;
using v3i32 = glm::i32vec3;
using v4i32 = glm::i32vec4;

using v2u32 = glm::u32vec2;
using v3u32 = glm::u32vec3;
using v4u32 = glm::u32vec4;

using mtx3 = glm::mat3x3;
using mtx4 = glm::mat4x4;

static constexpr v2f v2f_zero = v2f{0, 0};
static constexpr v2f v2f_one = v2f{1.0f, 1.0f};
static constexpr v2f v2f_up = v2f{0.0f, 1.0f};
static constexpr v2f v2f_left = v2f{-1.0f, 0.0f};
static constexpr v2f v2f_right = v2f{1.0f, 0.0f};
static constexpr v2f v2f_down = v2f{0.0f, -1.0f};

static constexpr v3f v3f_zero = v3f{0.0f, 0.0f, 0.0f};
static constexpr v3f v3f_one = v3f{1.0f, 1.0f, 1.0f};
static constexpr v3f v3f_forward = v3f{0.0f, 1.0f, 0.0f};
static constexpr v3f v3f_back = v3f{0.0f, -1.0f, 0.0f};

static constexpr v2i32 v2i32_zero = v2i32{0, 0};
static constexpr v2i32 v2i32_one = v2i32{1, 1};
static constexpr v2i32 v2i32_up = v2i32{0, 1};
static constexpr v2i32 v2i32_left = v2i32{-1, 0};
static constexpr v2i32 v2i32_right = v2i32{1, 0};
static constexpr v2i32 v2i32_down = v2i32{0, -1};


namespace vex
{
    static constexpr float pi{3.14159265358979323f};
    static constexpr float epsilon{1.e-8f};
    static constexpr float epsilon_half{1.e-4f};
    static constexpr float float_near_inf{3.4e+38f};

    static constexpr double double_pi{3.14159265358979323846264338327950288419716939};
    static constexpr double double_epsilon{1.e-8};
    static constexpr double double_half_epsilon{1.e-4};
    static constexpr double double_near_inf{3.4e+38};

    static constexpr float eulers{2.71828182845904523536f};
    static constexpr double double_eulers{2.718281828459045235360287471352662497757};

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

    constexpr float clampOne(float v)
    {
        v = v < 0 ? 0 : v;
        return v > 1 ? 1 : v;
    }

    inline float nearEqual(float val1, float val2, float eps = epsilon) /*fabs is not constexpr =( */
    {
        return fabsf(val1 - val2) <= epsilon;
    }
} // namespace vex