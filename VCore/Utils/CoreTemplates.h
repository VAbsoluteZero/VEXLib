#pragma once

#include <type_traits>

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;

using u8 = unsigned char;
using byte = unsigned char;

namespace vex::memory
{
	template <typename... Types>
	constexpr auto MaxSizeOf()
	{
		constexpr size_t list[sizeof...(Types)] = {sizeof(Types)...};
		auto It = &list[0];
		auto End = &list[sizeof...(Types)];
		auto Curmax = *It;
		if (It != End)
		{
			for (; It != End; ++It)
			{
				Curmax = (Curmax > *It) ? Curmax : *It;
			}
		}
		return Curmax;
	};

	template <typename... Types>
	constexpr auto MaxAlignOf()
	{
		constexpr size_t list[sizeof...(Types)] = {alignof(Types)...};
		auto It = &list[0];
		auto End = &list[sizeof...(Types)];
		auto Curmax = *It;
		if (It != End)
		{
			for (; It != End; ++It)
			{
				Curmax = (Curmax > *It) ? Curmax : *It;
			}
		}
		return Curmax;
	};
} // namespace vex::memory

namespace vex::traits
{
	template <typename TType, typename TFirstType, typename... TRest>
	constexpr size_t GetIndex()
	{
		if constexpr (!std::is_same<TType, TFirstType>::value)
			return 1 + GetIndex<TType, TRest...>();
		return 0;
	}
	template <typename TType>
	constexpr bool HasType()
	{
		return false;
	}
	template <>
	constexpr bool HasType<void>()
	{
		return false;
	}
	template <typename TType, typename TFirstType, typename... TRest>
	constexpr bool HasType()
	{
		if constexpr (!std::is_same<TType, TFirstType>::value)
			return HasType<TType, TRest...>();
		return true;
	}
	template <typename... TRest>
	constexpr bool AreAllTrivial()
	{
		return (... && std::is_trivial_v<TRest>);
	}

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

		static constexpr size_t Arity = sizeof...(Args);

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
		using ArgTypesT = typename std::remove_reference_t<typename ArgTypes<I, TTypeList<Args...>>::type>;
	};
} // namespace vex::traits
