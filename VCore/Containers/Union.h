#pragma once

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

		static constexpr byte kNullVal = 0xFF;


		template <typename T>
		static constexpr size_t Id()
		{
			constexpr auto typeIndex = traits::GetIndex<T, Types...>();
			return typeIndex;
		}

		bool HasAnyValue() const { return ValueIndex != kNullVal; }

		template <typename T>
		bool Has() const
		{
			constexpr auto typeIndex = traits::GetIndex<T, Types...>();
			return (typeIndex == this->ValueIndex);
		}

		template <typename T>
		T& GetUnchecked()
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
#if NDEBUG
			constexpr auto typeIndex = traits::GetIndex<T, Types...>();
			if (typeIndex != this->ValueIndex)
				std::abort();
#endif
			return *(reinterpret_cast<T*>(this->Storage));
		}

		template <typename T>
		const T& GetUnchecked() const
		{
			return this->template GetUnchecked<T>();
		}

		template <typename T>
		T* Find()
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			if (!UnionBase::Has<T>())
				return nullptr;

			return (reinterpret_cast<T*>(this->Storage));
		}

		template <typename T>
		const T* Find() const
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			if (!UnionBase::Has<T>())
				return nullptr;

			return (reinterpret_cast<const T*>(this->Storage));
		}

		template <typename TArg>
		inline void Match(void (*Func)(TArg&))
		{
			if (!UnionBase::Has<TArg>())
				return;
			Func((this)->template GetUnchecked<TArg>());
		}

		template <typename TFunc>
		inline void Match(TFunc Func)
		{
			using TraitsT = traits::FunctorTraits<decltype(Func)>;
			using TArg0 = std::remove_cv_t<typename TraitsT::template ArgTypesT<0>>;

			if (!UnionBase::Has<TArg0>())
				return;

			Func((this)->GetUnchecked<TArg0>());
		}

		template <typename... TFuncs>
		inline void MultiMatch(TFuncs&&... Funcs)
		{
			[](...) {}((Match(Funcs), 0)...);
		}

		void Reset()
		{
			((TSelf*)this)->DestroyValue();
			this->ValueIndex = kNullVal;
		}

		byte TypeIndex() const { return ValueIndex; }

	protected:
		byte ValueIndex = kNullVal; // intentionally first, aware of padding
		alignas(Alignment) byte Storage[SizeOfStorage];

		template <typename T>
		void SetTypeIndex()
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			constexpr auto typeIndex = traits::GetIndex<T, Types...>();
			this->ValueIndex = typeIndex;
		}
	};

	template <bool AreAllTrivial, typename... Types>
	struct UnionImpl;

	template <typename... Types>
	struct UnionImpl<true, Types...> : public UnionBase<UnionImpl<true, Types...>, Types...>
	{
		using Base = UnionBase<Types...>;
		friend struct UnionBase<Types...>;

		constexpr UnionImpl() = default;
		UnionImpl(const UnionImpl&) = default;

		template <typename T>
		UnionImpl(T&& Arg)
		{
			using TArg = std::remove_reference_t<T>;
			static_assert(traits::HasType<TArg, Types...>(), "Union cannot possibly contain this type");

			new (this->Storage) TArg(std::forward<T>(Arg));
			this->template SetTypeIndex<T>();
		}

		// #todo copy-assignment

		template <typename T>
		void Set(T&& Val)
		{
			using TArg = std::remove_reference_t<T>;
			static_assert(traits::HasType<TArg, Types...>(), "Union cannot possibly contain this type");

			*(reinterpret_cast<TArg*>(this->Storage)) = std::forward<T>(Val);
			this->template SetTypeIndex<T>();
		}

		//  reutrn default value if not there
		template <typename T>
		T GetValueOrDefault(T defaultVal) const
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");

			if (!this->template Has<T>())
				return defaultVal;

			return *(reinterpret_cast<const T*>(this->Storage));
		}

		// ok for primitives lets be like map and just construct the val if it is not there
		template <typename T>
		T& Get()
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			if (!this->template Has<T>())
				this->Set<T>(T());

			return *(reinterpret_cast<T*>(this->Storage));
		}

	private:
		// not calling dtor since storage restricted to PODs
		void DestroyValue() {}
	};


	template <typename... Types>
	struct UnionImpl<false, Types...> : public UnionBase<UnionImpl<false, Types...>, Types...>
	{
		using Base = UnionBase<Types...>;
		friend struct UnionBase<Types...>;

		using TSelf = UnionImpl<false, Types...>;
		constexpr UnionImpl() = default;

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
			std::cout << ">> INVOKED : "
					  << " [ CopyConstructor ] " << typeid(T).name() << "\n";
			new (self->Storage) T(other->template GetUnchecked<T>());
		}

		template <typename T>
		static void MoveConstructor(TSelf* self, TSelf* other)
		{ //
			std::cout << ">> INVOKED : "
					  << " [ MoveConstructor ] " << typeid(T).name() << "\n";
			new (self->Storage) T(std::move(other->template GetUnchecked<T>()));
			other->Reset();
		}

		template <typename T>
		static void CopyAssignment(TSelf* self, const TSelf* other)
		{ //
			std::cout << ">> INVOKED : "
					  << " [ == ] " << typeid(T).name() << "\n";
			self->template GetUnchecked<T>() = other->template GetUnchecked<T>();
		}

		template <typename T>
		static void MoveAssignment(TSelf* self, TSelf* other)
		{ //
			std::cout << ">> INVOKED : "
					  << " [ MoveAssignment ] " << typeid(T).name() << "\n";
			self->template GetUnchecked<T>() = std::move(other->template GetUnchecked<T>());
		}

		template <typename T>
		static void Destructor(TSelf* self)
		{ //
			std::cout << ">> INVOKED : "
					  << " [ Destructor ] " << typeid(T).name() << "\n";
			auto* value = reinterpret_cast<T*>(self->Storage);
			value->~T();
		}

		template <typename T>
		static constexpr MethodTable BuildMethodTable()
		{ //
			auto Table = MethodTable();

			Table.Destructor = &TSelf::Destructor<T>;

			Table.MoveConstruct = &TSelf::MoveConstructor<T>;
			Table.CopyConstruct = &TSelf::CopyConstructor<T>;

			Table.CopyAssignment = &TSelf::CopyAssignment<T>;
			Table.MoveAssignment = &TSelf::MoveAssignment<T>;

			return Table;
		}

		// maps type to method table
		static inline MethodTable gTables[sizeof...(Types)] = {BuildMethodTable<Types>()...};


		MethodTable* Resolve()
		{
			u32 Index = this->ValueIndex;
			return &gTables[Index];
		}

		void DestroyValue()
		{
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
			MethodTable* table = other->Resolve(this);
			table->MoveAssignment(this, other);
		}

	public:
		template <typename T>
		UnionImpl(T&& arg)
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			new (this->Storage) T(std::forward<T>(arg));
			this->template SetTypeIndex<T>();
		}
		UnionImpl(const UnionImpl& other)
		{ //
			if (other.HasAnyValue())
			{
				InvokeCopyCTOR(&other);
			}
			this->ValueIndex = other->ValueIndex;
		}

		UnionImpl(UnionImpl&& other)
		{ //
			if (other.HasAnyValue())
			{
				this->ValueIndex = other->ValueIndex;
				MoveConstructor(&other);
			}
			other.Reset();
		}

		UnionImpl& operator=(const UnionImpl& other)
		{
			if (this != &other)
			{
				InvokeCopyAssignment(&other);
				this->ValueIndex = other->ValueIndex;
			}

			return *this;
		}

		UnionImpl& operator=(UnionImpl&& other)
		{
			if (this != &other)
			{
				InvokeMoveAssignment(&other);
				this->ValueIndex = other->ValueIndex;
			}

			return *this;
		}

		~UnionImpl() { Reset(); }

		void Reset()
		{
			if (this->HasAnyValue())
				DestroyValue();

			this->ValueIndex = 0;
		}

		template <typename T>
		void Set(T&& Val)
		{
			using TArg = std::remove_reference_t<T>;
			static_assert(traits::HasType<TArg, Types...>(), "Union cannot possibly contain this type");

			if (this->HasAnyValue())
				Reset();

			new (this->Storage) TArg(std::forward<T>(Val));
			SetTypeIndex<TArg>();
		}

		template <typename T>
		void SetDefault()
		{
			using TArg = std::remove_reference_t<T>;
			if (this->template HasAnyValue())
				Reset();
			new (this->Storage) TArg();
			SetTypeIndex<TArg>();
		}
	};
} // namespace vex::union_impl

template <typename... Types>
using Union = vex::union_impl::UnionImpl<vex::traits::AreAllTrivial<Types...>(), Types...>;
