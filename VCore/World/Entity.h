#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/World/ComponentBase.h>
#include <VCore/Utils/TypeInfoUtils.h>  

namespace core 
{
	typedef int16_t tIDType;

	struct EntityHandle
	{
		tIDType ID = 0;

		EntityHandle() = default;
		~EntityHandle() = default;
		EntityHandle(const EntityHandle&) = default;
		explicit EntityHandle(int id) : ID(id) {}
		explicit EntityHandle(tIDType id) : ID(id) {}
		operator bool() const { return ID > 0; }
		inline int Hash() const { return ID; }
	};

	struct EntHandleHasher { static int Hash(const EntityHandle& h) { return h.Hash(); } };

	struct Entity
	{
		EntityHandle Handle; 

		tMask ComponentMask = 0; 

		template <class ...TTypes>
		inline bool Has() const noexcept
		{
			tMask buf = (... | TTypes::Mask); 
			return (ComponentMask & buf) == buf;
		}
		template <class ...TTypes>
		inline void SetComponentFlag() const noexcept
		{ 
			ComponentMask |= (... | TTypes::Mask);
		}

		operator EntityHandle() const { return Handle; }
		operator bool() const { return Handle.ID > 0; }
	};
} 