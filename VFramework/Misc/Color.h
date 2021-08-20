#pragma once

#include <VCore/Utils/VMath.h>

namespace vex
{
	struct Color
	{
		float r = 0;
		float g = 0;
		float b = 0;
		float a = 0;

		static constexpr Color Zero() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }
		static constexpr Color White() { return Color(1.f, 1.f, 1.f, 1.f); }
		static constexpr Color Gray() { return Color{0.5f, 0.5f, 0.5f, 1.f}; }
		static constexpr Color Black() { return Color(0.0f, 0.0f, 0.0f, 1.f); }

		static constexpr Color Red() { return Color{1.f, 0.0f, 0.0f, 1.f}; }

		static constexpr Color Green() { return Color{0.0f, 1.f, 0.0f, 1.f}; }

		static constexpr Color Blue() { return Color{0.0f, 0.0f, 1.f, 1.f}; }

		static constexpr Color Magenta() { return Color(1.f, 0.0f, 1.f, 1.f); }
		static constexpr Color Yellow() { return Color{1.f, 0.90f, 0.017f, 1.f}; }
		static constexpr Color Cyan() { return Color(0.0f, 1.f, 1.f, 1.f); }

		constexpr float Grayscale() { return (float)(0.299 * r + 0.587 * g + 0.114 * b); }

		constexpr Color(float red, float green, float blue, float alpha = 1)
		{
			r = red;
			g = green;
			b = blue;
			a = alpha;
		}

		constexpr Color(v4f v) : Color(v.x, v.y, v.z, v.w) {}
		constexpr Color(v3f v) : Color(v.x, v.y, v.z, 1) {}

		constexpr operator v4f() { return v4f(r, g, b, a); }
		constexpr Color operator*(const Color& o) { return Color(r * o.r, g * o.g, b * o.b, a * o.a); }
		constexpr Color operator*(float b) { return Color(r * b, g * b, b * b, a * b); }

		friend constexpr Color operator+(const Color& a, const Color& b)
		{
			return Color(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
		}
		friend constexpr Color operator-(const Color& a, const Color& b)
		{
			return Color(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a);
		}

		friend constexpr Color operator*(float b, Color a) { return Color(a.r * b, a.g * b, a.b * b, a.a * b); }
		friend constexpr Color operator*(Color a, float b) { return Color(a.r * b, a.g * b, a.b * b, a.a * b); }

		friend constexpr Color operator/(Color a, float b) { return Color(a.r / b, a.g / b, a.b / b, a.a / b); }

		friend constexpr bool operator==(Color a, Color b)
		{
			return (a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
		}

		friend constexpr bool operator!=(Color lhs, Color rhs) { return !(lhs == rhs); }

		static constexpr int Hash(const Color& c)
		{
			auto val = c.r + c.g * 13 + c.b * 131 + c.a * 1711;
			return (int)val;
		}
	};

	constexpr Color LerpUnclamped(Color first, Color second, float v)
	{
		return Color(first.r + (second.r - first.r) * v, first.g + (second.g - first.g) * v,
			first.b + (second.b - first.b) * v, first.a + (second.a - first.a) * v);
	}
	constexpr Color Lerp(Color first, Color second, float v) { return LerpUnclamped(first, second, ClampOne(v)); }
} // namespace vp
