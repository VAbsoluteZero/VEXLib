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
#include <VCore/Containers/SOAJoint.h>
#include <VCore/Utils/HashUtils.h>

#include <optional>
#include <vector>

#ifdef ECSCORE_x64
#include <VCore/Dependencies/fastmod.h>
#endif

#ifndef FORCE_INLINE
#if defined(_MSC_VER)

#define FORCE_INLINE __forceinline

#else // defined(_MSC_VER)

#define FORCE_INLINE inline __attribute__((always_inline))

#endif
#endif // ! FORCE_INLINE

// # TODO: add support for ALLOCATORS
// # TODO: implement shrink/realloc
namespace vex
{
	template <typename TKey>
	struct hasher
	{
		inline static int32_t Hash(const TKey& key)
		{
			// '-val' hash range is reserved for
			// empty records (effectively taking one bit from .Hash field)
			return (int)std::hash<TKey>{}(key);
		}
	};
	template <>
	struct hasher<int>
	{
		inline static int32_t Hash(const int& key) { return key; }
	};

	template <>
	struct hasher<std::string>
	{
		inline static int32_t Hash(const std::string& key)
		{
			return murmur::MurmurHash3_x86_32(key.data(), (int)key.size());
		}
	};
	struct DSentinel
	{
	};
	static constexpr const DSentinel kEndIteratorSentinel{};
	/*
	 * Simple and readable Hashtable/Map/Dictionary implementation.
	 * Loosely based on the .net (C#) dictionary implementation,
	 * optimized for fast insert/iteration/lookup.
	 * It preforms 1.5x to 10x faster than std::unordered_map in certain situations.
	 * It is about 2x to 8x faster than  UE's TMap .
	 * All around it is much faster to fill and iterate through it
	 * regardless of element size due to cache friendliness.
	 * Lookup for an element takes about the same time.
	 *
	 * Data storage itself is a flat SOA-like structure, all logical parts of the dictionary (buckets, recs..)
	 * are allocated inside same buffer/memory region.
	 * There are some checks to ensure that POD-like data is being processed as such (e.g. memcpy'd).
	 * This container does not require neither Key nor Val to have default constructor.
	 *
	 * NOTE (1): this container does NOT resemble unordered_map in a way it is allocated -
	 * it is using Flat memory buffer for all elements (instead of just storing pointers and
	 * allocating storage for elements later), so if you are using HUGE (lets say
	 * more than 2048 bytes) structures and lots of them - it could be better
	 * to use std version instead. Otherwise there could be spikes on alloc/realloc or free.
	 * Basically allocating 2MB+ upfront could be expensive.
	 */

	// #todo partially specialize for strings
	template <typename TKey, typename TVal, typename THasher = hasher<TKey>>
	class Dict
	{
	public:
		typedef TKey KeyType;
		typedef TVal ValueType;
		struct Record
		{
			TKey Key;
			TVal Value;
		};

		inline int32_t Size() const noexcept { return _top - _freeCount; }
		inline int32_t Capacity() const noexcept { return (int)_buckets.Capacity; }

		explicit Dict(uint32_t capacity = 7) : _data(util::ClosestPrimeSearch(capacity))
		{
			AssignNewBufferHandles();

			std::fill_n(_buckets.First, _buckets.size(), -1);
			auto b = ControlBlock{-1, -1};
			std::fill_n(_blocks.First, _blocks.size(), b);
		}

		Dict(const Dict& other) : _data(other.Capacity())
		{
			AssignNewBufferHandles();

			_top = other._top;
			_free = other._free;
			_freeCount = other._freeCount;

			RawBuffer<int32_t> otherBuckets = other._data.template GetBuffer<0, int32_t>();
			RawBuffer<ControlBlock> otherBlocks = other._data.template GetBuffer<1, ControlBlock>();
			RawBuffer<Record> otherRecs = other._data.template GetBuffer<2, Record>();

			otherBuckets.CopyTo(_buckets, {});
			otherBlocks.CopyTo(_blocks);

			if constexpr (std::is_trivially_copyable<Record>::value)
			{
				std::memcpy(_records.First, otherRecs.First, otherRecs.Capacity * sizeof(Record));
			}
			else
			{
				for (int32_t i = 0; i < otherRecs.size(); ++i)
				{
					if (otherBlocks[i].Hash >= 0)
					{
						new (&_records[i]) Record(otherRecs[i]);
					}
				}
			}
		}

		Dict& operator=(const Dict& other)
		{
			if (this != &other)
			{
				Dict tmp(other);
				*this = std::move(tmp);
			}

			return *this;
		}

		Dict& operator=(Dict&& other)
		{
			if (this != &other)
			{
				_data = std::move(other._data);

				_top = other._top;
				_free = other._free;
				_freeCount = other._freeCount;
				AssignNewBufferHandles();

				other._top = 0;
				other._freeCount = 0;
				other._free = 0;
				other._data = SOAOneBuffer<int, ControlBlock, Record>(util::ClosestPrimeSearch(3));

				other.AssignNewBufferHandles();
				std::fill_n(other._buckets.First, other._buckets.size(), -1);
				auto b = ControlBlock{-1, -1};
				std::fill_n(other._blocks.First, other._blocks.size(), b);
			}

			return *this;
		}

		~Dict()
		{
			if constexpr (!std::is_trivially_destructible<Record>::value)
			{
				for (int32_t i = 0; i < _blocks.size(); ++i)
				{
					if (_blocks[i].Hash >= 0)
						_records[i].~Record();
					_blocks[i].Hash = -1;
				}
			}
		}

		template <typename T = TVal>
		inline typename std::enable_if_t<std::is_default_constructible<T>::value, T*> Any()
		{
			if (Size() > 0)
			{
				for (int32_t i = 0; i < Size(); ++i)
				{
					if (_blocks[i].Hash > 0)
						return &_records[i].Value;
				}
			}

			return nullptr;
		}

		template <class... Types>
		inline void Emplace(const TKey& key, Types&&... arguments)
		{
			int32_t i = FindRec(key);
			if (i >= 0)
			{
				_records[i].Value.~TVal();
				new (&_records[i].Value) TVal(std::forward<Types>(arguments)...);
			}
			else
			{
				Record& r = CreateRecord(key);
				new (&r.Value) TVal(std::forward<Types>(arguments)...);
			}
		}
		static inline volatile int32_t dbg;
		template <class... Types>
		inline TVal& EmplaceAndGet(const TKey& key, Types&&... arguments)
		{
			int32_t i = FindRec(key);
			if (i >= 0)
			{
				_records[i].Value.~TVal();
				new (&_records[i].Value) TVal(std::forward<Types>(arguments)...);
				return _records[i].Value;
			}
			else
			{
				Record& r = CreateRecord(key);
				new (&r.Value) TVal(std::forward<Types>(arguments)...);
				return r.Value;
			}
		}

		template <typename T = TVal>
		inline typename std::enable_if_t<std::is_default_constructible<T>::value, TVal> ValueOrDefault(
			const TKey& key) const
		{
			int32_t ind = FindRec(key);
			return ind >= 0 ? _records[ind].Value : TVal();
		}

		inline TVal* TryGet(const TKey& key) const noexcept
		{
			int32_t ind = FindRec(key);
			return ind >= 0 ? &_records[ind].Value : nullptr;
		}

		bool Remove(const TKey& key) noexcept
		{
			int32_t hashCode = THasher::Hash(key) & 0x7FFFFFFF;
			int32_t bucket = mod(hashCode, _buckets.size());
			int32_t previous = -1;
			for (int32_t i = _buckets[bucket]; i >= 0; previous = i, i = _blocks[i].Next)
			{
				if (_blocks[i].Hash == hashCode && (_records[i].Key == key))
				{
					// only
					if (previous < 0)
					{
						_buckets[bucket] = _blocks[i].Next;
					}
					// middle or last
					else
					{
						_blocks[previous].Next = _blocks[i].Next;
					}

					Record& entry = _records[i];
					{
						_blocks[i].Hash = -1;
						_blocks[i].Next = _free;

						entry.Key.~TKey();
						entry.Value.~TVal();
					}
					_free = i;
					_freeCount++;

					return true;
				}
			}

			return false;
		}

		inline void Clear()
		{
			if (_top == 0) // it is already empty
				return;

			_freeCount = 0;
			_free = -1;
			_top = 0;

			if constexpr (std::is_trivially_copyable<Record>::value)
			{
				// basically do nothing
			}
			else
			{
				for (int32_t i = 0; i < _blocks.size(); ++i)
				{
					if (_blocks[i].Hash >= 0)
						_records[i].~Record();
				}
			}

			std::fill_n(_buckets.First, _buckets.size(), -1);
			auto b = ControlBlock{-1, -1};
			std::fill_n(_blocks.First, _blocks.size(), b);
		}


		struct DIterator
		{
			FORCE_INLINE bool Advance()
			{
				int32_t count = _map.Size();
				while (_index < (count - 1))
				{
					_index++;
					ControlBlock& e = _map._blocks[_index];
					if (e.Hash >= 0)
					{
						return true;
					}
				}

				_index = _map.Size() + 1;
				return false;
			}

			FORCE_INLINE bool IsDone() const { return _index >= (_map.Size()); }

			friend auto operator==(DIterator lhs, DSentinel rhs) { return lhs.IsDone(); }
			friend auto operator==(DSentinel lhs, DIterator rhs) { return rhs == lhs; }
			friend auto operator!=(DIterator lhs, DSentinel rhs) { return !(lhs == rhs); }
			friend auto operator!=(DSentinel lhs, DIterator rhs) { return !(lhs == rhs); }

			inline Record& operator*() const { return Current(); }
			inline auto& operator++()
			{
				Advance();
				return *this;
			}

			inline Record& Current() const
			{
				// assert(_index < _map.Size());
				return _map._records[_index];
			}

		private:
			DIterator(const Dict& owner) : _map(owner) {}
			const Dict& _map;
			int32_t _index = 0;
			;

			friend class Dict;
		};

		FORCE_INLINE DIterator begin() noexcept { return DIterator{*this}; };
		FORCE_INLINE DSentinel end() const noexcept { return kEndIteratorSentinel; };

		template <typename T = TVal>
		inline typename std::enable_if_t<std::is_default_constructible<T>::value, T&> operator[](const TKey& key)
		{
			int32_t i = FindRec(key);

			if (i < 0)
			{
				Record& r = CreateRecord(key);
				new (&r.Value) TVal();
				return r.Value;
			}

			return _records[i].Value;
		}
		FORCE_INLINE bool Contains(const TKey& item) const { return FindRec(item) >= 0; }

	private:
		FORCE_INLINE int32_t FindRec(const TKey& key) const noexcept
		{
			// ensure abs value
			int32_t hashCode = THasher::Hash(key) & 0x7FFFFFFF;
			int32_t bucketIndex = mod(hashCode, (int)_buckets.Capacity);
			for (int32_t i = _buckets[bucketIndex]; i >= 0; i = _blocks[i].Next)
			{
				if (_blocks[i].Hash == hashCode)
					if (_records[i].Key == key)
						return i;
			}
			return -1;
		}
		struct ControlBlock
		{
			int32_t Hash = -1;
			int32_t Next = -1;
		};

#ifdef ECSCORE_x64
		uint64_t _fastmodM = 0;
#endif
		int32_t _free = 0;
		int32_t _freeCount = 0;
		int32_t _top = 0;

		const float kGrowFactor = 1.6f;

		SOAOneBuffer<int, // buckets
			ControlBlock,	// hash and index to next, POD
			Record>			// Record, trivial or not
			_data;

		RawBuffer<int32_t> _buckets;
		RawBuffer<Record> _records;
		RawBuffer<ControlBlock> _blocks;

		inline void AssignNewBufferHandles()
		{
			_buckets = _data.template GetBuffer<0, int32_t>();
			_blocks = _data.template GetBuffer<1, ControlBlock>();
			_records = _data.template GetBuffer<2, Record>();

#ifdef ECSCORE_x64
			_fastmodM = fastmod::computeM_s32(Capacity());
#endif
		}

		FORCE_INLINE int32_t mod(int32_t a, int32_t b) const noexcept
		{
#ifdef ECSCORE_x64
			return fastmod::fastmod_s32(a, _fastmodM, b);
#else
			return a % b;
#endif
		}

		void Grow()
		{
			using namespace util;
			int32_t newSize = ClosestPrimeSearch((int)(_buckets.size() * kGrowFactor + 1));

			SOAOneBuffer<int, ControlBlock, Record> newData(newSize);
			RawBuffer<Record> newRecs = newData.template GetBuffer<2, Record>();

			if constexpr (std::is_trivially_copyable<Record>::value)
			{
				std::memcpy(newRecs.First, _records.First, _records.Capacity * sizeof(Record));
			}
			else if constexpr (std::is_move_constructible<Record>::value)
			{
				for (int32_t i = 0; i < _records.size(); ++i)
				{
					if (_blocks[i].Hash >= 0)
					{
						new (&newRecs[i]) Record(std::move(_records[i]));
						_records[i].~Record();
					}
				}
			}
			else
			{
				for (int32_t i = 0; i < _records.size(); ++i)
				{
					if (_blocks[i].Hash >= 0)
					{
						new (&newRecs[i]) Record(_records[i]);
						_records[i].~Record();
					}
				}
			}

			_blocks.CopyTo(newData.template GetBuffer<1, ControlBlock>(), ControlBlock{});

			_data = std::move(newData);

			AssignNewBufferHandles();
			std::fill_n(_buckets.First, _buckets.size(), -1);

			for (int32_t i = 0; i < _top; i++)
			{
				if (_blocks[i].Hash >= 0)
				{
					int32_t bucket = mod(_blocks[i].Hash, newSize); // == hash % size

					_blocks[i].Next = _buckets[bucket];
					_buckets[bucket] = i; // old i-th element hash would not lead here
				}
			}
		}

		inline Record& CreateRecord(const TKey& key)
		{
			int32_t hashCode = THasher::Hash(key) & 0x7FFFFFFF;
			int32_t bucketIndex = mod(hashCode, (int)_buckets.Capacity);
			int32_t index = 0;

			if (_freeCount > 0)
			{
				index = _free;
				_free = _blocks[index].Next;
				_freeCount--;
			}
			else
			{
				// capacity reached
				if (_top == _records.size())
				{
					Grow();
					bucketIndex = mod(hashCode, (int)_buckets.Capacity);
					;
				}
				index = _top;
				_top++;
			}

			Record& entry = _records[index];
			{
				_blocks[index].Hash = hashCode;
				_blocks[index].Next = _buckets[bucketIndex];

				new (&entry.Key) TKey(key);
				// value should be initialized in getter
			}
			_buckets[bucketIndex] = index;
			return entry;
		}
	};
	;
} // namespace vex
