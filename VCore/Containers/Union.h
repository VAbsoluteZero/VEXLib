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

namespace vex::union_impl
{
	template <typename TSelf, typename... Types>
	struct UnionBase
	{
		static_assert(sizeof...(Types) <= 7, "too many types in Union");
		static constexpr auto SizeOfStorage = vex::memory::MaxSizeOf<Types...>();
		static constexpr auto Alignment = vex::memory::MaxAlignOf<Types...>();
		static constexpr auto TypeCount = sizeof...(Types);

		static constexpr byte kNullVal = vex::traits::kTypeIndexNone;

		template <typename T>
		static constexpr size_t Id()
		{
			constexpr auto typeIndex = traits::GetIndex<T, Types...>();
			return typeIndex;
		}

		bool HasAnyValue() const noexcept { return _valueIndex != kNullVal; }

		template <typename T>
		bool Has() const noexcept
		{
			constexpr auto typeIndex = traits::GetIndex<T, Types...>();
			return typeIndex == _valueIndex;
		}

		template <typename T>
		T& GetUnchecked() noexcept
		{
			using TArg = std::remove_reference_t<T>;
			static_assert(traits::HasType<TArg, Types...>(), "Union cannot possibly contain this type");
#if !NDEBUG
			constexpr auto typeIndex = traits::GetIndex<TArg, Types...>();
			if (typeIndex != this->_valueIndex)
				std::abort();
#endif
			return *(reinterpret_cast<TArg*>(this->_storage));
		}

		template <typename T>
		const T& GetUnchecked() const noexcept
		{
			using TArg = std::remove_reference_t<T>;
			static_assert(traits::HasType<TArg, Types...>(), "Union cannot possibly contain this type");
#if !NDEBUG
			constexpr auto typeIndex = traits::GetIndex<TArg, Types...>();
			if (typeIndex != this->_valueIndex)
				std::abort();
#endif
			return *(reinterpret_cast<const TArg*>(this->_storage));
		}

		template <typename T>
		T* Find() noexcept
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			if (!UnionBase::Has<T>())
				return nullptr;

			return (reinterpret_cast<T*>(this->_storage));
		}

		template <typename T>
		const T* Find() const noexcept
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			if (!UnionBase::Has<T>())
				return nullptr;

			return (reinterpret_cast<const T*>(this->_storage));
		}

		template <typename TArg>
		inline void Match(void (*Func)(TArg&&))
		{
			using TUnderlying = std::decay_t<TArg>;
			if (!UnionBase::Has<TUnderlying>())
				return;

			Func((this)->GetUnchecked<TUnderlying>());
		}

		template <typename TFunc>
		inline void Match(TFunc Func)
		{
			using TraitsT = traits::FunctorTraits<decltype(Func)>;
			using TArg0 = std::decay_t<typename TraitsT::template ArgTypesT<0>>;

			if (!UnionBase::Has<TArg0>())
				return;

			Func((this)->GetUnchecked<TArg0>());
		}

		template <typename... TFuncs>
		inline void MultiMatch(TFuncs&&... Funcs)
		{
			[](...) {
			}((Match(Funcs), 0)...);
		}

		inline void Reset() noexcept
		{
			if (!HasAnyValue())
				return;
			((TSelf*)this)->DestroyValue();
			this->_valueIndex = kNullVal;
		}

		byte TypeIndex() const noexcept { return _valueIndex; }

		operator bool() const noexcept { return HasAnyValue(); }

	protected:
		byte _valueIndex = kNullVal; // intentionally first, aware of padding
		alignas(Alignment) byte _storage[SizeOfStorage];

		template <typename T>
		inline void SetTypeIndex() noexcept
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			constexpr auto typeIndex = traits::GetIndex<T, Types...>();
			this->_valueIndex = typeIndex;
		}
	};

	template <bool AreAllTrivial, typename... Types>
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

		static constexpr bool IsTrivial = true;

		template <typename T>
		UnionImpl(T&& Arg) noexcept
		{
			using TUnderlying = std::decay_t<T>;
			static_assert(traits::HasType<TUnderlying, Types...>(), "Union cannot possibly contain this type");

			new (this->_storage) TUnderlying(std::forward<T>(Arg));
			this->template SetTypeIndex<T>();
		}

		template <typename T, typename TArg = T>
		inline void Set(TArg&& Val) noexcept
		{
			constexpr bool kConv = std::is_convertible_v<TArg, T>; // (... || std::is_convertible_v<T, Types>);
			static_assert(kConv || traits::HasType<TArg, Types...>(), "Union cannot possibly contain this type");

			if (std::is_same_v<T, TArg>)
			{
				*(reinterpret_cast<T*>(this->_storage)) = std::forward<std::decay_t<TArg>>(Val);
			}
			else // convert
			{
				*(reinterpret_cast<T*>(this->_storage)) = std::forward<TArg>(Val);
			}
			this->template SetTypeIndex<T>();
		}
		template <typename T>
		inline void Set(T&& Val) noexcept
		{
			using TUnderlying = std::decay_t<T>;

			static_assert(traits::HasType<TUnderlying, Types...>(), "Union cannot possibly contain this type");

			*(reinterpret_cast<TUnderlying*>(this->_storage)) = std::forward<T>(Val);
			this->template SetTypeIndex<T>();
		}

		//  reutrn default value if not there
		template <typename T>
		inline T GetValueOrDefault(T defaultVal) const noexcept
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");

			if (!this->template Has<T>())
				return defaultVal;

			return *(reinterpret_cast<const T*>(this->_storage));
		}

		// ok for primitives lets be like map and just construct the val if it is not there
		template <typename T>
		inline T& Get() noexcept
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			if (!this->template Has<T>())
				this->Set<T>(T());

			return *(reinterpret_cast<T*>(this->_storage));
		}

	private:
		// not calling dtor since storage restricted to PODs
		inline void DestroyValue() noexcept {}
	};


	template <typename... Types>
	struct UnionImpl<false, Types...> : public UnionBase<UnionImpl<false, Types...>, Types...>
	{
		using Base = UnionBase<UnionImpl<false, Types...>, Types...>;
		friend struct UnionBase<UnionImpl<false, Types...>, Types...>;

		using TSelf = UnionImpl<false, Types...>;
		constexpr UnionImpl() = default;

		static constexpr bool IsTrivial = false;

	public:
		UnionImpl(const UnionImpl& other)
		{ //
			static_assert((... & std::is_move_constructible_v<Types>), "Union contains non-movable type");
			if (other.HasAnyValue())
			{
				InvokeCopyCTOR(this, &other);
			}
			this->_valueIndex = other._valueIndex;
		}

		UnionImpl& operator=(const UnionImpl& other)
		{
			if (this != &other)
			{
				if (!other.HasAnyValue())
				{
					this->Reset();
					return *this;
				}
				if (this->_valueIndex == other._valueIndex)
					InvokeCopyAssignment(&other);
				else
				{
					this->Reset();
					this->_valueIndex = other._valueIndex;
					InvokeCopyCTOR(other);
				};
			}

			return *this;
		}

		UnionImpl& operator=(UnionImpl&& other)
		{
			if (this != &other)
			{
				if (!other.HasAnyValue())
				{
					this->Reset();
					return *this;
				}

				if (this->_valueIndex == other._valueIndex)
					InvokeMoveAssignment(&other);
				else
				{
					this->Reset();
					this->_valueIndex = other._valueIndex;
					InvokeMoveCTOR(&other);
				}
			}

			return *this;
		}
		template <typename T>
		UnionImpl(T&& arg)
		{
			using TUnderlying = std::decay_t<T>;

			if constexpr (std::is_same_v<TUnderlying, TSelf>)
			{
				if (arg.HasAnyValue())
				{
					this->_valueIndex = arg._valueIndex;
					InvokeMoveCTOR(&arg);
					arg.Reset();
				}
			}
			else
			{
				static_assert(traits::HasType<TUnderlying, Types...>(), "Union cannot possibly contain this type");
				new (this->_storage) TUnderlying(std::forward<T>(arg));
				this->template SetTypeIndex<TUnderlying>();
			};
		}

		~UnionImpl() { this->Reset(); }

		template <typename TargetType, typename TArg = TargetType>
		void Set(TArg&& Val)
		{
			constexpr bool kConv = std::is_convertible_v<TArg, TargetType>; // (... || std::is_convertible_v<T, Types>);
			static_assert(kConv || traits::HasType<TargetType, Types...>(), // ? #todo better check
				"Union cannot possibly contain this type");

			if constexpr (std::is_same_v<TargetType, TArg>)
			{
				if (this->template Has<TargetType>())
					*(reinterpret_cast<TargetType*>(this->_storage)) = std::forward<TargetType>(Val);
				else
				{
					if (this->HasAnyValue())
						this->Reset();
					new (this->_storage) TargetType(std::forward<TargetType>(Val));
				}
			}
			else // convert
			{
				if (this->template Has<TargetType>())
					*(reinterpret_cast<TargetType*>(this->_storage)) = std::forward<TArg>(Val);
				else
				{
					if (this->HasAnyValue())
						this->Reset();
					new (this->_storage) TargetType(std::forward<TArg>(Val));
				}
			}
			this->template SetTypeIndex<TargetType>();
		}

		template <typename T>
		void SetDefault()
		{
			using TArg = std::decay_t<T>;
			if (this->template HasAnyValue())
				this->Reset();
			new (this->_storage) TArg();
			this->template SetTypeIndex<TArg>();
		}

	private:
		// table of ctor/dtor's to be resolved based on type index
		struct MethodTable
		{
			void (*CopyConstruct)(TSelf* self, const TSelf* other);
			void (*CopyAssignment)(TSelf* self, const TSelf* other);
			void (*MoveConstruct)(TSelf* self, TSelf* other);
			void (*MoveAssignment)(TSelf* self, TSelf* other);
			void (*Destructor)(TSelf* self);
		};

		template <typename T>
		static void CopyConstructor(TSelf* self, const TSelf* other)
		{ //
			new (self->_storage) T(other->template GetUnchecked<T>());
		}

		template <typename T>
		static void MoveConstructor(TSelf* self, TSelf* other)
		{ //
			new (self->_storage) T(std::move(other->template GetUnchecked<T>()));
			other->Reset();
		}

		template <typename T>
		static void CopyAssignment(TSelf* self, const TSelf* other)
		{ //
			self->template GetUnchecked<T>() = other->template GetUnchecked<T>();
		}

		template <typename T>
		static void MoveAssignment(TSelf* self, TSelf* other)
		{ //
			self->template GetUnchecked<T>() = std::move(other->template GetUnchecked<T>());
		}

		template <typename T>
		static void Destructor(TSelf* self)
		{ //
			auto* value = reinterpret_cast<T*>(self->_storage);
			value->~T();
		}

		template <typename T>
		static constexpr MethodTable BuildMethodTable()
		{ //
			auto table = MethodTable();

			table.Destructor = &TSelf::Destructor<T>;

			table.MoveConstruct = &TSelf::MoveConstructor<T>;
			table.CopyConstruct = &TSelf::CopyConstructor<T>;

			table.CopyAssignment = &TSelf::CopyAssignment<T>;
			table.MoveAssignment = &TSelf::MoveAssignment<T>;

			return table;
		}

		// maps type to method table
		static inline MethodTable gTables[sizeof...(Types)] = {BuildMethodTable<Types>()...};


		MethodTable* Resolve() noexcept
		{
			assert(Base::HasAnyValue());
			u32 index = this->_valueIndex;
			return &gTables[index];
		}

		void DestroyValue() noexcept
		{
			// this ptr is source of type info
			MethodTable* table = this->Resolve();
			table->Destructor(this);
		}

		void InvokeCopyCTOR(const TSelf* other)
		{
			// other is source of type info
			MethodTable* table = other->Resolve();
			table->CopyConstruct(this, other);
		}
		void InvokeCopyAssignment(const TSelf* other)
		{
			// other is source of type info
			MethodTable* table = other->Resolve();
			table->CopyConstruct(this, other);
		}

		void InvokeMoveCTOR(TSelf* other)
		{
			// other is source of type info
			MethodTable* table = other->Resolve();
			table->MoveConstruct(this, other);
		}
		void InvokeMoveAssignment(TSelf* other)
		{
			// other is source of type info
			MethodTable* table = other->Resolve();
			table->MoveAssignment(this, other);
		}
	};
} // namespace vex::union_impl

namespace vex
{
	template <typename... Types>
	using Union = vex::union_impl::UnionImpl<vex::traits::AreAllTrivial<Types...>(), Types...>;

	// todo write proper option that is efficient.
	template <typename T>
	using Option = vex::union_impl::UnionImpl<std::is_trivial_v<T>, T>;
} // namespace vex
