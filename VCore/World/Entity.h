#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/Utils/TypeInfoUtils.h>
#include <VCore/World/ComponentBase.h>

namespace vex
{
	using tIDType = int16_t;

	struct EntityHandle
	{
		tIDType ID = 0;

		EntityHandle() = default;
		~EntityHandle() = default;
		EntityHandle(const EntityHandle&) = default;
		explicit EntityHandle(int id) : ID(id) {}
		explicit EntityHandle(tIDType id) : ID(id) {}
		operator bool() const { return ID > 0; }
		inline int hash() const { return ID; }
	};

	struct EntHandleHasher
	{
		static int hash(const EntityHandle& h) { return h.hash(); }
	};

	struct Entity
	{
		EntityHandle handle;

		tMask ComponentMask = 0;

		template <class... TTypes>
		inline bool Has() const noexcept
		{
			tMask buf = (... | TTypes::Mask);
			return (ComponentMask & buf) == buf;
		}
		template <class... TTypes>
		inline void SetComponentFlag() const noexcept
		{
			ComponentMask |= (... | TTypes::Mask);
		}

		operator EntityHandle() const { return handle; }
		operator bool() const { return handle.ID > 0; }
	};
} // namespace vex
