#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
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
#include "VCore/Utils/VUtilsBase.h"


namespace vex::union_impl
{
    template <typename TSelf, typename... Types>
    struct UnionBase
    {
        static_assert(sizeof...(Types) <= 16, "too many types in Union");
        static constexpr auto size_of_storage = vex::maxSizeOf<Types...>();
        static constexpr auto alignment = vex::maxAlignOf<Types...>();
        static constexpr auto type_cnt = sizeof...(Types);

        static constexpr byte k_null_type = vex::traits::type_index_none;

        template <typename T>
        static constexpr size_t id()
        {
            constexpr auto type_index = traits::template getIndex<T, Types...>();
            return type_index;
        }

        constexpr bool hasAnyValue() const noexcept { return value_index != k_null_type; }

        template <typename T>
        constexpr bool has() const noexcept
        {
            constexpr auto typeIndex = traits::getIndex<T, Types...>();
            return typeIndex == value_index;
        }

        template <typename T>
        constexpr T& get() & noexcept
        {
            using TArg = std::remove_reference_t<T>;
            static_assert(traits::hasType<TArg, Types...>(), "Union cannot possibly contain this type");
            constexpr auto typeIndex = traits::getIndex<TArg, Types...>();
            checkAlways_(typeIndex == this->value_index);
            // printf("&\n");
            return *(reinterpret_cast<TArg*>(this->storage));
        }

        template <typename T>
        constexpr const T& get() const& noexcept
        {
            using TArg = std::remove_reference_t<T>;
            static_assert(traits::hasType<TArg, Types...>(), "Union cannot possibly contain this type");
            constexpr auto typeIndex = traits::getIndex<TArg, Types...>();
            checkAlways_(typeIndex == this->value_index);
            // printf("const &\n");
            return *(reinterpret_cast<const TArg*>(this->storage));
        }

        template <typename T>
        constexpr T&& get() && noexcept
        {
            using TArg = std::remove_reference_t<T>;
            static_assert(traits::hasType<TArg, Types...>(), "Union cannot possibly contain this type");
            constexpr auto typeIndex = traits::getIndex<TArg, Types...>();
            checkAlways_(typeIndex == this->value_index);
            // printf("&&\n");
            return static_cast<TArg&&>(*(reinterpret_cast<TArg*>(this->storage)));
        }
        template <typename T>
        constexpr T&& get() const&& noexcept
        {
            using TArg = std::remove_reference_t<T>;
            static_assert(traits::hasType<TArg, Types...>(), "Union cannot possibly contain this type");
            constexpr auto typeIndex = traits::getIndex<TArg, Types...>();
            checkAlways_(typeIndex == this->value_index);
            // printf("const &&\n");
            return static_cast<TArg&&>(*(reinterpret_cast<TArg*>(this->storage)));
        }

        template <u32 idx>
        constexpr auto* saticFindByIdx()
        {
            static_assert(idx < type_cnt, "index is out of bounds");
            using T = typename vex::GetTypeByIndex<idx, Types...>::type;
            return find<std::decay_t<T>>();
        }

        template <typename T>
        T* find() noexcept
        {
            static_assert(traits::hasType<T, Types...>(), "Union cannot possibly contain this type");
            if (!UnionBase::has<T>())
                return nullptr;

            return (reinterpret_cast<T*>(this->storage));
        }

        template <typename T>
        const T* find() const noexcept
        {
            static_assert(traits::hasType<T, Types...>(), "Union cannot possibly contain this type");
            if (!UnionBase::has<T>())
                return nullptr;

            return (reinterpret_cast<const T*>(this->storage));
        }

        template <typename TArg>
        inline void matchOne(void (*Func)(TArg&&))
        {
            using TUnderlying = std::decay_t<TArg>;
            if (!UnionBase::has<TUnderlying>())
                return;

            Func((this)->get<TUnderlying>());
        }

        template <typename TFunc>
        inline constexpr void matchOne(TFunc&& Func)
        {
            using TraitsT = traits::FunctorTraits<std::decay_t<decltype(Func)>>;
            using TArg0 = std::decay_t<typename TraitsT::template ArgTypesT<0>>;

            if (!UnionBase::has<TArg0>())
                return;
            else
                Func((this)->get<TArg0>());
        }

        template <typename... TFuncs>
        inline constexpr void match(TFuncs&&... Funcs)
        {
            [](...)
            {
            }((matchOne(Funcs), 0)...);
        }

        // helper macro for unrolling switch in 'visit'
#define VEX_visitCase(num)                         \
    case num:                                      \
        if constexpr (num < type_cnt)              \
        {                                          \
            return Func(*(saticFindByIdx<num>())); \
        }
        // !VEX_visitCase
        template <typename TFunc>
        inline constexpr auto visit(TFunc&& Func)
        {
            using TRetType = decltype(Func(*(saticFindByIdx<0>())));

            switch (this->value_index)
            {
                VEX_PASTE_INC_16(0, VEX_visitCase);
            }

            if constexpr (!std::is_void_v<TRetType>)
            {
                return TRetType{};
            }
        }
#undef VEX_visitCase

        inline void reset() noexcept
        {
            if (!hasAnyValue())
                return;
            ((TSelf*)this)->destroyValue();
            this->value_index = k_null_type;
        }

        constexpr byte typeIndex() const noexcept { return value_index; }

    protected:
        byte value_index = k_null_type;
        alignas(alignment) byte storage[size_of_storage];

        template <typename T>
        inline void setTypeIndex() noexcept
        {
            static_assert(traits::hasType<T, Types...>(), "Union cannot possibly contain this type");
            constexpr auto typeIndex = traits::getIndex<T, Types...>();
            this->value_index = typeIndex;
        }
    };

    template <bool are_all_trivial, typename... Types>
    struct UnionImpl;

    template <typename... Types>
    struct UnionImpl<true, Types...> : public UnionBase<UnionImpl<true, Types...>, Types...>
    {
        using Base = UnionBase<UnionImpl<true, Types...>, Types...>;
        friend struct UnionBase<UnionImpl<true, Types...>, Types...>;

        constexpr UnionImpl() = default;
        UnionImpl(const UnionImpl&) = default;
        UnionImpl(UnionImpl&&) = default;

        UnionImpl& operator=(const UnionImpl&) = default;
        UnionImpl& operator=(UnionImpl&&) = default;

        static constexpr bool is_trivial = true;

        template <typename T>
        UnionImpl(T&& Arg) noexcept
        {
            using TUnderlying = std::decay_t<T>;
            static_assert(
                traits::hasType<TUnderlying, Types...>(), "Union cannot possibly contain this type");

            new (this->storage) TUnderlying(std::forward<T>(Arg));
            this->template setTypeIndex<TUnderlying>();
        }

        template <typename T, typename TArg = T>
        inline void set(TArg&& Val) noexcept
        {
            constexpr bool kConv =
                std::is_convertible_v<TArg, T>; // (... || std::is_convertible_v<T, Types>);
            static_assert(
                kConv || traits::hasType<TArg, Types...>(), "Union cannot possibly contain this type");

            if (std::is_same_v<T, TArg>)
            {
                *(reinterpret_cast<T*>(this->storage)) = std::forward<std::decay_t<TArg>>(Val);
            }
            else // convert
            {
                *(reinterpret_cast<T*>(this->storage)) = std::forward<TArg>(Val);
            }
            this->template setTypeIndex<T>();
        }
        template <typename T>
        inline void set(T&& Val) noexcept
        {
            static_assert(traits::hasType<T, Types...>(), "Union cannot possibly contain this type");

            *(reinterpret_cast<T*>(this->storage)) = std::forward<T>(Val);
            this->template setTypeIndex<T>();
        }

        //  reutrn default value if not there
        template <typename T>
        inline T getValueOrDefault(T defaultVal) const noexcept
        {
            static_assert(traits::hasType<T, Types...>(), "Union cannot possibly contain this type");

            if (!this->template has<T>())
                return defaultVal;

            return *(reinterpret_cast<const T*>(this->storage));
        }

        // ok for primitives lets be like map and just construct the val if it is not there
        template <typename T>
        inline T& getOrAdd() noexcept
        {
            static_assert(traits::hasType<T, Types...>(), "Union cannot possibly contain this type");
            if (!this->template has<T>())
                this->set<T>(T());

            return *(reinterpret_cast<T*>(this->storage));
        }

    private:
        // not calling dtor since storage restricted to PODs
        inline void destroyValue() noexcept {}
    };

    // struct HelperSwitch

    template <typename... Types>
    struct UnionImpl<false, Types...> : public UnionBase<UnionImpl<false, Types...>, Types...>
    {
        using Base = UnionBase<UnionImpl<false, Types...>, Types...>;
        friend struct UnionBase<UnionImpl<false, Types...>, Types...>;

        using TSelf = UnionImpl<false, Types...>;
        constexpr UnionImpl() = default;

        static constexpr bool is_trivial = false;

    private:
        template <typename TargetType>
        constexpr auto castStorage()
        {
            return (reinterpret_cast<TargetType*>(this->storage));
        }
        template <typename TargetType>
        constexpr const auto castStorage() const
        {
            return (reinterpret_cast<const TargetType*>(this->storage));
        }

        template <typename T>
        void destroyIter()
        {
            if (Base::template has<T>())
            {
                (castStorage<T>())->~T();
                Base::value_index = Base::k_null_type;
            }
        }
        inline void destroyValue() noexcept
        { //
            (destroyIter<Types>(), ...);
        }

        template <typename T, typename Other>
        constexpr void copyIter(Other&& other)
        {
            if (other.template has<T>())
            {
                auto other_storage = other.template castStorage<T>();
                new (castStorage<T>()) T(*other_storage);
                this->value_index = other.value_index;
            }
        }
        template <typename T, typename Other>
        constexpr void moveIter(Other&& other)
        {
            if (other.template has<T>())
            {
                auto other_storage = other.template castStorage<T>();
                new (castStorage<T>()) T(std::move(*other_storage));
                this->value_index = other.value_index;
                other.value_index = Base::k_null_type;
            }
        }
        template <typename T, typename Other>
        constexpr void copyAssignIter(Other&& other)
        {
            if (other.template has<T>())
            {
                auto other_storage = other.template castStorage<T>();
                *(castStorage<T>()) = (*other_storage);
                this->value_index = other.value_index;
            }
        }
        template <typename T, typename Other>
        constexpr void moveAssignIter(Other&& other)
        {
            if (other.template has<T>())
            {
                auto other_storage = other.template castStorage<T>();
                *(castStorage<T>()) = (std::move(*other_storage));
                this->value_index = other.value_index;
                other.value_index = Base::k_null_type;
            }
        }

    public:
        ~UnionImpl() { this->destroyValue(); }
        UnionImpl(const UnionImpl& other)
        { //
            static_assert((... & std::is_move_constructible_v<Types>), "Union contains non-movable type");
            if (other.hasAnyValue())
            {
                (copyIter<Types>(other), ...);
            }
        }

        UnionImpl& operator=(const UnionImpl& other)
        {
            if (this != &other)
            {
                if (!other.hasAnyValue())
                {
                    this->reset();
                    return *this;
                }
                if (this->value_index == other.value_index)
                {
                    (copyAssignIter<Types>(other), ...);
                }
                else
                {
                    this->reset();
                    (copyIter<Types>(other), ...);
                };
            }

            return *this;
        }

        UnionImpl& operator=(UnionImpl&& other)
        {
            if (this != &other)
            {
                if (!other.hasAnyValue())
                {
                    this->reset();
                    return *this;
                }

                if (this->value_index == other.value_index)
                {
                    (moveAssignIter<Types>(other), ...);
                }
                else
                {
                    this->reset();
                    (moveIter<Types>(other), ...);
                }
            }

            return *this;
        }
        template <typename TypeArg>
        UnionImpl& operator=(TypeArg&& arg)
        {
            if (this != (void*)&arg)
            {
                using TUnderlying = std::decay_t<TypeArg>;
                set<TUnderlying, TypeArg>(std::forward<TypeArg>(arg));
            }

            return *this;
        }

        template <typename T>
        UnionImpl(T&& arg)
        {
            using TUnderlying = std::decay_t<T>;

            if constexpr (std::is_same_v<TUnderlying, TSelf>)
            {
                if (arg.hasAnyValue())
                {
                    (moveIter<Types>(arg), ...);
                }
            }
            else
            {
                static_assert(
                    traits::hasType<TUnderlying, Types...>(), "Union cannot possibly contain this type");
                new (this->storage) TUnderlying(std::forward<T>(arg));
                this->template setTypeIndex<TUnderlying>();
            };
        }

        template <typename TargetType, typename TArg = TargetType>
        void set(TArg&& Val)
        {
            constexpr bool kConv =
                std::is_convertible_v<TArg, TargetType>; // (... || std::is_convertible_v<T, Types>);
            static_assert(kConv || traits::hasType<TargetType, Types...>(), // ? #todo better check
                "Union cannot possibly contain this type");

            if constexpr (std::is_same_v<TargetType, TArg>)
            {
                if (this->template has<TargetType>())
                    *(reinterpret_cast<TargetType*>(this->storage)) = std::forward<TargetType>(Val);
                else
                {
                    if (this->hasAnyValue())
                        this->reset();
                    new (this->storage) TargetType(std::forward<TargetType>(Val));
                }
            }
            else // convert
            {
                if (this->template has<TargetType>())
                    *(reinterpret_cast<TargetType*>(this->storage)) = std::forward<TArg>(Val);
                else
                {
                    if (this->hasAnyValue())
                        this->reset();
                    new (this->storage) TargetType(std::forward<TArg>(Val));
                }
            }
            this->template setTypeIndex<TargetType>();
        }

        template <typename T>
        T getMoved() noexcept
        {
            using TArg = std::remove_reference_t<T>;
            static_assert(traits::hasType<TArg, Types...>(), "Union cannot possibly contain this type");

            constexpr auto typeIndex = traits::getIndex<TArg, Types...>();
            checkAlways_(typeIndex == this->value_index);

            auto val = reinterpret_cast<TArg*>(this->storage);
            return std::move(*val);
        }

        template <typename T>
        void setDefault()
        {
            using TArg = std::decay_t<T>;
            if (this->template hasAnyValue())
                this->reset();
            new (this->storage) TArg();
            this->template setTypeIndex<TArg>();
        }
    };
} // namespace vex::union_impl

namespace vex
{
    template <typename... Types>
    using Union = vex::union_impl::UnionImpl<vex::traits::are_all_trivial_v<Types...>, Types...>;

    // todo write proper option that is efficient.
    template <typename T>
    using Option = vex::union_impl::UnionImpl<std::is_trivial_v<T>, T>;
} // namespace vex
