#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2020 Vladyslav Joss
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "VCore/Utils/CoreTemplates.h"

namespace vex
{
    template <auto Index, typename TStoredType>
    struct ValueHolder
    {
        template <typename T>
        ValueHolder(T&& InValue) : Value(std::forward<T>(InValue))
        {
        }

        ValueHolder() = default;
        ValueHolder(ValueHolder&&) = default;
        ValueHolder(const ValueHolder&) = default;
        ValueHolder& operator=(const ValueHolder&) = default;

        TStoredType Value;
    };

    template <typename... Types>
    struct Wrap
    {
        template <typename T, int... N>
        struct CreateMembers;

        template <int... Number>
        struct CreateMembers<std::integer_sequence<int, Number...>> : public ValueHolder<Number, Types>...
        {
            static constexpr auto MemberCount = sizeof...(Types);

            CreateMembers(Types... val) : ValueHolder<Number, Types>(val)... {}

            template <typename... TArgs>
            CreateMembers(TArgs&&... val) : ValueHolder<Number, Types>(std::forward<TArgs>(val))...
            {
            }

            CreateMembers& operator=(const CreateMembers&) = default;
            CreateMembers& operator=(CreateMembers&&) = default;
            CreateMembers(const CreateMembers&) = default;
            CreateMembers(CreateMembers&&) = default;
            CreateMembers() = default;

            template <int I>
            constexpr auto& get() &
            {
                static_assert(I < (sizeof...(Types)), "out of bounds");
                using Target = typename GetTypeByIndex<I, Types...>::type;
                return ((static_cast<ValueHolder<I, Target>*>(this))->Value);
            }
            template <int I>
            constexpr const auto& get() const&
            {
                static_assert(I < (sizeof...(Types)), "out of bounds");
                using Target = typename GetTypeByIndex<I, Types...>::type;
                return ((static_cast<const ValueHolder<I, Target>*>(this))->Value);
            }
            template <int I>
            constexpr auto get() &&
            {
                static_assert(I < (sizeof...(Types)), "out of bounds");
                using Target = typename GetTypeByIndex<I, Types...>::type;
                return static_cast<Target&&>((static_cast<ValueHolder<I, Target>*>(this))->Value);
            }
            template <int I>
            constexpr auto get() const&&
            {
                static_assert(I < (sizeof...(Types)), "out of bounds");
                using Target = typename GetTypeByIndex<I, Types...>::type;
                return static_cast<Target&&>((static_cast<ValueHolder<I, Target>*>(this))->Value);
            }
        };

        using WrappedMembers = CreateMembers<std::make_integer_sequence<int, sizeof...(Types)>>;
        static inline decltype(auto) MakeDefault()
        {
            return CreateMembers<std::make_integer_sequence<int, sizeof...(Types)>>();
        }

        WrappedMembers Members;
    };

    template <typename... Types>
    using TupleAlias = decltype(Wrap<Types...>::MakeDefault());

    template <typename... Types>
    struct Tuple : public TupleAlias<Types...>
    {
        Tuple(const Tuple&) = default;
        Tuple(Tuple& other) : Tuple{const_cast<Tuple const&>(other)} {};
        Tuple(Tuple&&) = default;
        Tuple(Types... val) : TupleAlias<Types...>((val)...) {}
        template <typename... TArgs>
        Tuple(TArgs&&... val) : TupleAlias<Types...>(std::forward<TArgs>(val)...)
        {
            static_assert(sizeof...(TArgs) == sizeof...(Types), //
                "Provide arguments for every subtype constructor, or none.");
        }
        Tuple() = default;
        ~Tuple() = default;

        Tuple& operator=(const Tuple&) = default;
        Tuple& operator=(Tuple&&) = default;
    };
} // namespace vex

namespace std
{
    template <typename... Types>
    struct tuple_size<vex::Tuple<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)>
    {
    };

    template <std::size_t N, class... Types>
    struct tuple_element<N, vex::Tuple<Types...>>
    {
        using type = typename vex::GetTypeByIndex<N, Types...>::type;
    };

    template <auto I, class... Types>
    constexpr decltype(auto) get(vex::Tuple<Types...>& arg)
    {
        return arg.template get<I>();
    }
    template <auto I, class... Types>
    constexpr decltype(auto) get(const vex::Tuple<Types...>& arg)
    {
        return arg.template get<I>();
    }
    template <auto I, class... Types>
    constexpr decltype(auto) get(vex::Tuple<Types...>&& arg)
    {
        return arg.template get<I>();
    }
    template <auto I, class... Types>
    constexpr decltype(auto) get(const vex::Tuple<Types...>&& arg)
    {
        return arg.template get<I>();
    }
} // namespace std
