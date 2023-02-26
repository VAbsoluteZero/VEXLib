#pragma once
#include <cstdint>
#include <type_traits>

#ifndef FORCE_INLINE
    #if defined(_MSC_VER) 
        #define FORCE_INLINE __forceinline 
    #else // defined(_MSC_VER) 
        #define FORCE_INLINE inline __attribute__((always_inline)) 
    #endif
#endif // ! FORCE_INLINE

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;

using f32 = float;
using f64 = double;

using u8 = unsigned char;
using byte = unsigned char;

#define VEX_PASTE_INC_4(StartIndex, Body) \
    Body(StartIndex);                     \
    Body(StartIndex + 1);                 \
    Body(StartIndex + 2);                 \
    Body(StartIndex + 3)
// ^ last ';' is ommited intentionally
#define VEX_PASTE_INC_16(StartIndex, Body) \
    VEX_PASTE_INC_4(StartIndex, Body);     \
    VEX_PASTE_INC_4(StartIndex + 4, Body); \
    VEX_PASTE_INC_4(StartIndex + 8, Body); \
    VEX_PASTE_INC_4(StartIndex + 12, Body)
#define VEX_PASTE_INC_64(StartIndex, Body)   \
    VEX_PASTE_INC_16(StartIndex, Body);      \
    VEX_PASTE_INC_16(StartIndex + 16, Body); \
    VEX_PASTE_INC_16(StartIndex + 32, Body); \
    VEX_PASTE_INC_16(StartIndex + 48, Body)


namespace vex
{
    enum class EGenericStatus
    {
        Failure,
        Success
    };

    struct Error
    {
    };
} // namespace vex

namespace std // needed to avoid <tuple> deps while using str bindings
{
    template <class _Tuple>
    struct tuple_size
    {
    };

    template <size_t _Index, class _Tuple>
    struct tuple_element
    {
    };
} // namespace std

namespace vex
{
    template <typename... Types>
    constexpr auto maxSizeOf()
    {
        constexpr size_t list[sizeof...(Types)] = {sizeof(Types)...};
        auto it = &list[0];
        auto range_end = &list[sizeof...(Types)];
        auto curmax = *it;
        for (; it != range_end; ++it)
        {
            curmax = (curmax > *it) ? curmax : *it;
        }
        return curmax;
    };

    template <typename... Types>
    constexpr auto maxAlignOf()
    {
        constexpr size_t list[sizeof...(Types)] = {alignof(Types)...};
        auto it = &list[0];
        auto range_end = &list[sizeof...(Types)];
        auto curmax = *it;
        for (; it != range_end; ++it)
        {
            curmax = (curmax > *it) ? curmax : *it;
        }
        return curmax;
    };
} // namespace vex


namespace vex::traits
{
    static constexpr size_t type_index_none = 0xff;
    template <typename TType>
    constexpr size_t getTypeIndexInList() // #todo => rename
    {
        return type_index_none;
    }
    template <typename TType, typename TFirstType, typename... TRest>
    constexpr size_t getTypeIndexInList()
    {
        if constexpr (!std::is_same<TType, TFirstType>::value)
            return 1 + getTypeIndexInList<TType, TRest...>();
        return 0;
    }

    template <typename TType, typename... TRest>
    constexpr size_t getIndex()
    {
        constexpr auto val = getTypeIndexInList<TType, TRest...>();
        return val > type_index_none ? type_index_none : val;
    }

    template <typename TType>
    constexpr bool hasType()
    {
        return false;
    }
    template <>
    constexpr bool hasType<void>()
    {
        return false;
    }
    template <typename TType, typename TFirstType, typename... TRest>
    constexpr bool hasType()
    {
        if constexpr (!std::is_same<TType, TFirstType>::value)
            return hasType<TType, TRest...>();
        return true;
    }
    template <typename... TRest>
    constexpr bool areAllTrivial()
    {
        return (... && std::is_trivial_v<TRest>);
    }
    template <typename... TRest>
    static constexpr bool are_all_trivial_v = (... && std::is_trivial_v<TRest>);


    template <typename T, typename... TRest>
    constexpr bool isConvertible()
    {
        return (... && std::is_convertible_v<T, TRest>);
    }
    namespace private_impl
    {
        template <bool hash_conversion, typename T, typename... TRest>
        struct SelectConvertibleTarget
        {
        };
        template <typename T, typename... TRest>
        struct SelectConvertibleTarget<false, T, TRest...>
        {
            using Type = void;
        };
        template <typename T, typename... TRest>
        struct SelectConvertibleTarget<true, T, TRest...>
        {
            using Type = void;
        };
    } // namespace private_impl

    template <typename T, typename... TRest>
    using ConvertionType = private_impl::SelectConvertibleTarget<isConvertible<T, TRest...>, T, TRest...>;

    template <typename... TArgs>
    struct TTypeList
    {
    };

    template <typename T>
    struct FunctorTraits : public FunctorTraits<decltype(&T::operator())>
    {
    };

    template <typename ClassType, typename ReturnType, typename... Args>
    struct FunctorTraits<ReturnType (ClassType::*)(Args...) const>
    {
        typedef ReturnType TResult;

        static constexpr size_t arity = sizeof...(Args);

        template <std::size_t I, typename T>
        struct ArgTypes;

        template <std::size_t I, typename Head, typename... Tail>
        struct ArgTypes<I, TTypeList<Head, Tail...>> : ArgTypes<I - 1, TTypeList<Tail...>>
        {
            typedef Head type;
        };

        template <typename Head, typename... Tail>
        struct ArgTypes<0, TTypeList<Head, Tail...>>
        {
            typedef Head type;
        };

        template <std::size_t I>
        using ArgTypesT = typename std::decay_t<typename ArgTypes<I, TTypeList<Args...>>::type>;
    }; 
} // namespace vex::traits 

namespace vex
{
    namespace impl
    {
        struct vxSentinel
        {
        };
    } // namespace impl
    static constexpr impl::vxSentinel k_seq_end = impl::vxSentinel{};

    template <class T, bool add = true>
    struct AddConst;
    template <class T>
    struct AddConst<T, true>
    {
        using type = const T;
    };
    template <class T>
    struct AddConst<T, false>
    {
        using type = T;
    };

    template <std::size_t index, typename Cur, typename... Args>
    struct GetTypeByIndex
    {
        using type = typename GetTypeByIndex<index - 1, Args...>::type;
    };
    template <typename Cur, typename... Args>
    struct GetTypeByIndex<0, Cur, Args...>
    {
        using type = Cur;
    };

    template <std::size_t index, typename T>
    struct GetTypeByIndexUnwrap;

    template <std::size_t index, template <typename...> typename Wrapper, typename... Args>
    struct GetTypeByIndexUnwrap<index, Wrapper<Args...>>
    {
        using type = typename GetTypeByIndex<index, Args...>::type;
    };

    template <int range_start, int range_end>
    struct CRange
    {
        static constexpr int32_t st = range_start;

        static constexpr int32_t signum(int32_t v) { return (0 < v) - (v < 0); }
        static constexpr int32_t step = signum(range_end - range_start);

        struct SqIterator
        {
            friend auto operator==(SqIterator lhs, impl::vxSentinel rhs) { return lhs.isDone(); }
            friend auto operator==(impl::vxSentinel lhs, SqIterator rhs) { return rhs == lhs; }
            friend auto operator!=(SqIterator lhs, impl::vxSentinel rhs) { return !(lhs == rhs); }
            friend auto operator!=(impl::vxSentinel lhs, SqIterator rhs) { return !(lhs == rhs); }
            bool isDone() const { return current == range_end; }
            inline int operator*() const { return current; }
            inline auto& operator++()
            {
                current += step;
                return current;
            }
            int32_t current = st;
        };

        SqIterator begin() noexcept { return SqIterator{}; };
        impl::vxSentinel end() const noexcept { return k_seq_end; };
    };

    template <int range_end>
    using ZeroTo = CRange<0, range_end>;

    struct Range
    {
        static constexpr const impl::vxSentinel k_seq_end = impl::vxSentinel{};
        friend auto operator==(Range lhs, impl::vxSentinel rhs) { return lhs.isDone(); }
        friend auto operator==(impl::vxSentinel lhs, Range rhs) { return rhs == lhs; }
        friend auto operator!=(Range lhs, impl::vxSentinel rhs) { return !(lhs == rhs); }
        friend auto operator!=(impl::vxSentinel lhs, Range rhs) { return !(lhs == rhs); }

        bool isDone() const { return current >= range_end; }
        inline int operator*() const { return current; }

        inline auto& operator++()
        {
            current++;
            return *this;
        }

        Range() = delete;
        Range(int e) : range_end(e) {}
        Range(int s, int e) : range_start(s), range_end(e) {}
        Range(int c, int s, int e) : range_start(s), range_end(e), current(c) {}
        Range(const Range&) = default;

        int range_start = 0;
        int range_end = 0;
        int current = 0;

        Range begin() noexcept { return Range{0, range_start, range_end}; };
        impl::vxSentinel end() const noexcept { return k_seq_end; };
    };

    inline Range operator"" _times(unsigned long long x) { return Range((int)x); }
     
    template <typename T>
    void zeroInit(T* arg)
    {
        memset(&arg, 0, sizeof(std::decay_t<T>));
    }

    template <typename T> // use with auto: auto v = makeZeroed<POD>(); RVO should make it free.
    inline T makeZeroed() noexcept
    {
        T outval;
        memset(&outval, 0, sizeof(std::decay_t<T>));
        return outval;
    }
} // namespace vex

#define vexZeroInit(x) vex::zeroInit(x) 