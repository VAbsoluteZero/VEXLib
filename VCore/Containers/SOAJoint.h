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
#include <vector> 
#include <array>  
#include <cassert>
#include <initializer_list>
#include <type_traits>
#include <VCore/Utils/HashUtils.h>

namespace core
{
	inline constexpr int roundUp(int n, int m)
	{
		return ((n + m - 1) / m) * m;
	}
	/* this is just mutable span/region of SOABuffer*/
	template<typename T>
	struct RawBuffer
	{
		T* First = nullptr;
		T* End = nullptr;
		size_t Capacity = 0;

		inline int size() const { return (int)Capacity; }
		const T* begin() const { return First; }
		const T* end() const { return End; }
		FORCE_INLINE T& operator[](int i) const
		{
			return *(First + i);
		}

		void CopyTo(const RawBuffer<T>& other)
		{ 
			for (int i = 0; i < Capacity && i < other.Capacity; i++)
			{
				other[i] = (*this)[i];
			}
		}
		void CopyTo(const RawBuffer<T>& other, const T& fillVal)
		{
			for (auto i = 0; i < Capacity && i < other.Capacity; i++)
			{
				other[i] = (*this)[i];
			}

			for (int i = (int)this->Capacity;  i < other.Capacity; i++)
			{
				other[i] = fillVal;
			}
		} 

		RawBuffer(T* first, size_t capacity) :
			First(first), 
			End(first + capacity), 
			Capacity(capacity)
		{ 
		}
		RawBuffer() {};

		RawBuffer(const RawBuffer& x) :
			First(x.First),
			End(x.First + x.Capacity),
			Capacity(x.Capacity)
		{};
		RawBuffer(RawBuffer&& a) = delete;
		RawBuffer& operator=(const RawBuffer& a) = default;
		RawBuffer& operator=(RawBuffer&& a) = default;
		~RawBuffer() = default;
	};

	// use it to allocate several buffers in one continuous memory region
	// DOES NOT CALL THE DESTRUCTORS OF AN ELEMENT, call it yourself. 
	// It does free() the memory region itslef.
	// use only as inner container
	template<typename... TTypes>
	class SOAJointBuffer
	{
		static const size_t kAlignment = 32;
	public: 
		static constexpr size_t ArrayCount = sizeof...(TTypes);
		struct Block
		{ 
			size_t Capacity = 0; 
			size_t SizeOf = 0;
			size_t FirstByte = 0;
			void* First = nullptr; 
			template<typename T>
			T* Start() const
			{
				return reinterpret_cast<T*>(First); 
			}
		};  

		template<size_t arrayId, typename T>
		inline T& Get(int elementIndex) const
		{
			return *(std::get<arrayId>(_blocks).template Start<T>() + elementIndex);
		} 

		template<int N, typename... Ts> using NthTypeOf = 
			typename std::tuple_element<N, std::tuple<Ts...>>::type;

		template<size_t arrayId, typename T>
		inline void EmplaceNewAt(int i, const T& val)
		{
			static_assert(arrayId < ArrayCount, "accessing an element outside of bounds");
			static_assert(std::is_same<NthTypeOf<arrayId, TTypes...>, T>::value,
				"SOAJoint::Add<__I__, __TYPE__>(elem) -> invalid type when accessing element of SOA"); 

			auto& v = std::get<arrayId>(_blocks);
			new (v.template Start<T>() + i)
				T(val); 
		}
		template<size_t arrayId, typename T, typename... TArgs>
		inline void EmplaceNewAt(int i, TArgs... args)
		{
			static_assert(arrayId < ArrayCount, "accessing an element outside of bounds");
			static_assert(std::is_same<NthTypeOf<arrayId, TTypes...>, T>::value,
				"SOAJoint::EmplaceBack<__I__, __TYPE__>(args) -> invalid type when accessing element of SOA");

			auto& v = std::get<arrayId>(_blocks);
			new (v.template Start<T>() + i)
				T(std::forward<TArgs>(args)...); 
		}
		
		template<
			size_t arrayId,
			typename T>
		inline RawBuffer<T> GetBuffer() const noexcept
		{
			static_assert(arrayId < ArrayCount, "accessing an element outside of bounds");
			static_assert(std::is_same<NthTypeOf<arrayId, TTypes...>, T>::value,
				"SOAJoint::GetView<__I__, __TYPE__>(void) -> invalid type when accessing sub array of SOA");

			const Block& v = std::get<arrayId>(_blocks);
			return RawBuffer<T>( v.template Start<T>(), v.Capacity );
		} 

		SOAJointBuffer() = delete;

		explicit SOAJointBuffer(size_t capacity) noexcept
		{
			assert(capacity > 0);
				//"0 cap makes no sense except you want to break something"
			for (int i = 0; i < _blocks.size(); ++i)
			{ 
				_blocks[i].Capacity = capacity;
			}
			CreateBuffer();
		}  

		//explicit SOAJointBuffer()
		//{
		//	_allocated = false;
		//}

		SOAJointBuffer(const SOAJointBuffer& other) = delete;
		SOAJointBuffer(SOAJointBuffer&& other) noexcept
		{
			*this = std::move(other);
		}
		SOAJointBuffer& operator=(SOAJointBuffer&& other) noexcept
		{
			if (this != &other)
			{
				free(_mem);
				_mem = other._mem;
				_allocated = other._allocated;
				_byteCnt = other._byteCnt;
				_blocks = std::move(other._blocks);

				other._mem = nullptr;
				other._byteCnt = 0;
				other._allocated = 0;
			}

			return *this;
		}

		~SOAJointBuffer()
		{
			//Destroy<0, TTypes...>();
			free(_mem);
		}
	private:
		explicit SOAJointBuffer(const std::array<size_t, ArrayCount>& capacities)
		{
			assert(capacities.size() == ArrayCount);//"initializer_list size and count of arrays should always match"
			for (int i = 0; i < _blocks.size(); ++i)
			{
				if (i < capacities.size())
					_blocks[i].Capacity = *(capacities.begin() + i);
				else
					_blocks[i].Capacity = 0;
			}
			CreateBuffer();
		}
		std::array<Block, sizeof...(TTypes)> _blocks{ Block{0, sizeof(TTypes), 0, nullptr}... };

		void* _mem = nullptr;
		size_t _byteCnt = 0;
		bool _allocated = false;

		inline void CreateBuffer()
		{
			size_t size = 0;
			size_t currentStart = 0;
			for (int i = 0; i < _blocks.size(); ++i)
			{
				Block& b = _blocks[i];
				currentStart = (size_t)roundUp((int)size, kAlignment); // #todo write override guard to padding
				size = currentStart;
				b.FirstByte = currentStart;

				size += b.SizeOf * b.Capacity;
			}
			 
			_mem = malloc(size + kAlignment);

			size_t addr = (size_t)_mem + kAlignment; // 80(+64) = 144 (16off)
			void* actualStart = (void*)(addr - (addr % kAlignment));

			for (int i = 0; i < _blocks.size(); ++i)
			{
				Block& b = _blocks[i];
				b.First = ((char*)actualStart + b.FirstByte);
			}
			_byteCnt = size;

			_allocated = true;
		}
	};  
}