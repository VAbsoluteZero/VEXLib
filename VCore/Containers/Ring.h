#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */

#include<vector>
namespace vex
{
	template<int N, class T>
	class Ring
	{
	public:
		int LastIndex()
		{
			int i = _first - _size;
			if (i < 0)
			{
				i = N - _size;
			}
			assert(i >= 0 && i < N);
			assert(N >= _size);

			return i;
		}

		bool Full()
		{
			return (N - 1) == _size;
		}

		bool Empty() { return 0 == _size; }
		int Size() { return _size; }

		template<typename TFwd = T>
		T& Put(TFwd&& item)
		{
			_first++;
			if (_size < N)
			{
				_size++;
			}

			if (_first >= N)
			{
				_first = 0;
			}

			_items[_first] = std::forward<TFwd>(item);
			return _items[_first];
		}

		T&& Pop()
		{
			assert(_size > 0);

			int i = _first;
			--_size;

			if (--_first < 0)
			{
				_first = N - 1;
			}

			return std::move(_items[i]);
		}

		T& Peek()
		{
			return _items[_first];
		}

		Ring()
		{

			_first = _size = 0;
		}
	private:
		std::array<T, N> _items;
		int _size = 0;
		int _first = 0;
	};
}