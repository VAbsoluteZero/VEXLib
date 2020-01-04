#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */ 
#include <VCore/Containers/Dict.h>
 
#include <VCore/World/Entity.h>

namespace core 
{
	class StorageBase
	{
	public:
		StorageBase(tMask m, tTypeID tid) : kMask(m), kTypeID(tid) {};
		virtual ~StorageBase() = 0 {};
		virtual bool Remove(EntityHandle h) = 0;
		virtual bool Clone(EntityHandle original, EntityHandle newOne) = 0;

		virtual std::unique_ptr<StorageBase> DuplicateSelf() = 0;
		virtual bool MoveFrom(StorageBase* other) = 0;

		virtual void Clear() = 0;

		const tMask kMask;
		const tTypeID kTypeID;
	};
	// for singleton-like data
	// distinct type to intentionally prevent accidental 
	// addition of type(say int) as Global
	class IGlobal
	{
	public:
		virtual ~IGlobal() = default;
	};

	// ! #todo change API to resemble Chandler Carruth's idea of hashtable api  
	template<class TComp>
	class Storage final : public StorageBase
	{ 
	public:  
		inline TComp* Find(EntityHandle handle) const noexcept
		{
			return _data.TryGet(handle);
		}

		static inline TComp* Find(const Storage<TComp>& store, EntityHandle handle) noexcept
		{
			return store.Find(handle);
		}

		template <typename T = TComp>
		inline typename std::enable_if_t<std::is_default_constructible<T>::value, T&>
		Get(EntityHandle handle)
		{
			return _data[handle];
		}

		template <typename T = TComp>
		inline typename std::enable_if_t<std::is_default_constructible<T>::value, T*>
		First()
		{
			return _data.Any();
		}

		template<class ...TArgs>
		inline TComp& Emplace(EntityHandle handle, TArgs&&... arguments)
		{
			// create or replace
			return _data.EmplaceAndGet(handle, std::forward<TArgs>(arguments)...);
		}

		// Inherited via StorageBase 
		virtual bool Clone(EntityHandle original, EntityHandle newOne) override
		{
			if (!_data.Contains(original)) return false;
			TComp& ncomp = *_data.TryGet(original);
			_data.Emplace(newOne, ncomp);;
			return true;
		}
		virtual bool Remove(EntityHandle handle) override 
		{
			return _data.Remove(handle);
		} 

		virtual void Clear() override
		{
			_data.Clear();
		}

		struct DataEnumerable
		{
			auto begin() { return _data.begin(); }
			auto end() { return _data.end(); } 

		private:
			DataEnumerable(const Storage& owner) : _owner(owner) {}
			const Storage& _owner;
			friend class Storage;
		};

		DataEnumerable Enumerable() { return *this; } //implicit conversion to enumerable 
		 
		const int GenID = 0; 

		// Inherited via StorageBase
		virtual std::unique_ptr<StorageBase> DuplicateSelf() override
		{
			Storage* inst = new Storage();
			inst->_data = _data;
			std::unique_ptr<StorageBase> ret(inst);
			return std::move(ret);
		} 

		virtual bool MoveFrom(StorageBase* other) override 
		{
			if (!other || other->kMask != kMask)
				return false;

			// should be safe to do after above type check,
			// and it is only way if RTTI is disabled
			Storage* casted = static_cast<Storage*>(other);
			_data = std::move(casted->_data);
			return true;
		}


		Storage() : StorageBase(TComp::Mask, tinfo::typeID<TComp>())
		{ 
		} 

		virtual ~Storage() {}
	private: 
		// underlying container may (and should) be replaced to tailor-made storage,
		// this will do for now though. Usually I would have direct lookup table and raw
		// array with it, but since this (Dict) hashtable implementation
		// stores elements in continuous memory region thats ok.
		Dict<EntityHandle, TComp, EntHandleHasher> _data; 
	};
}