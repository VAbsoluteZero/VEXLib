#pragma once

#include <glm_common.h>

#include <glm/ext/matrix_transform.hpp>

#ifndef FORCE_INLINE
#if defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#else // defined(_MSC_VER)
#define FORCE_INLINE inline __attribute__((always_inline))
#endif
#endif // ! FORCE_INLINE

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

static constexpr mtx3 mtx3_identity = glm::identity<mtx3>();
static constexpr mtx4 mtx4_identity = glm::identity<mtx4>();

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

static constexpr v4f v4f_zero = v4f{0.0f, 0.0f, 0.0f, 0.0f};
static constexpr v4f v4f_one = v4f{1.0f, 1.0f, 1.0f, 1.0f};

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

    FORCE_INLINE constexpr float cross2d(v3f a, v3f b) { return a.x * b.y - b.x * a.y; }

    // so there is no need to include giant <algorithm> just for max
    template <typename T>
    FORCE_INLINE constexpr T max(T a, T b)
    {
        return a > b ? a : b;
    }
    template <typename T>
    FORCE_INLINE constexpr T min(T a, T b)
    {
        return a > b ? b : a;
    }

    FORCE_INLINE constexpr float clampOne(float v)
    {
        v = v < 0 ? 0 : v;
        return v > 1 ? 1 : v;
    }

    // std::lerp is 2x slower in debug, same speed in release
    template <typename T>
    FORCE_INLINE constexpr T lerp(T v0, T v1, float t)
    {
        return (1 - t) * v0 + t * v1;
    }

    template <typename T>
    FORCE_INLINE constexpr T lerpClamped(T v0, T v1, float t)
    {
        float ct = clampOne(t);
        return (1 - ct) * v0 + ct * v1;
    }

    FORCE_INLINE constexpr float nearEqual(
        float val1, float val2, float eps = epsilon) /*fabs is not constexpr =( */
    {
        return fabsf(val1 - val2) <= epsilon;
    }
} // namespace vex