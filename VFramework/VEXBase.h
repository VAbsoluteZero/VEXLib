#pragma once

#include <spdlog/spdlog.h>
//
#include <VCore/Utils/CoreTemplates.h>
#include <VCore/Utils/TTraits.h>
#include <VCore/Utils/VMath.h>
#include <VCore/Utils/VUtilsBase.h>
//
#include <VCore/Containers/Array.h>
#include <VCore/Containers/Tuple.h>
//
#include <memory>
#include <string>
#include <vector>

// using DynVal = vex::Union<i32, f32, bool, v3f, v2i32, const char*, std::string>;
template <>
struct fmt::formatter<v2i32>
{
    // Parses format specifications of the form ['f' | 'e'].
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const v2i32& p, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};
template <>
struct fmt::formatter<v2f>
{
    // Parses format specifications of the form ['f' | 'e'].
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const v2f& p, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};
template <>
struct fmt::formatter<v3f> : fmt::formatter<double>
{
    template <typename FormatContext>
    auto format(const v3f& p, FormatContext& ctx) const -> decltype(ctx.out())
    {
        ctx.advance_to(detail::write(ctx.out(), '('));
        ctx.advance_to(formatter<double>::format(p.x, ctx));
        ctx.advance_to(detail::write(ctx.out(), ", "));
        ctx.advance_to(formatter<double>::format(p.y, ctx));
        ctx.advance_to(detail::write(ctx.out(), ", "));
        ctx.advance_to(formatter<double>::format(p.z, ctx));
        ctx.advance_to(detail::write(ctx.out(), ')'));
        return ctx.out() ;
    }
};