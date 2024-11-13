

#if defined(GLM_VERSION_MAJOR) && defined(VEX_USES_SPDLOG)

template <>
struct fmt::formatter<v2i32> {
    // Parses format specifications of the form ['f' | 'e'].
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const v2i32& p, FormatContext& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};
template <>
struct fmt::formatter<v2f> {
    // Parses format specifications of the form ['f' | 'e'].
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const v2f& p, FormatContext& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};
template <>
struct fmt::formatter<v3f> : fmt::formatter<double> {
    template <typename FormatContext>
    auto format(const v3f& p, FormatContext& ctx) const -> decltype(ctx.out()) {
        ctx.advance_to(detail::write(ctx.out(), '('));
        ctx.advance_to(formatter<double>::format(p.x, ctx));
        ctx.advance_to(detail::write(ctx.out(), ", "));
        ctx.advance_to(formatter<double>::format(p.y, ctx));
        ctx.advance_to(detail::write(ctx.out(), ", "));
        ctx.advance_to(formatter<double>::format(p.z, ctx));
        ctx.advance_to(detail::write(ctx.out(), ')'));
        return ctx.out();
    }
};

#endif