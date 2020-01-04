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
#include <VCore/World/World.h>

using namespace core;

bool core::World::RegisterArchetype(const std::string& id,
	tfArchetypeBuilder builder, bool replace)
{  
	if (!replace && _archetypes.Contains(id))
		return false;

	_archetypes.Emplace(id, builder);

	return true;
}

EntityHandle core::World::InstantiateArchetype(const std::string& id)
{
	if (!_archetypes.Contains(id))
		return EntityHandle{};

	// #todo fixme
	EntityHandle newOne = CreateBlank("-of archetype-");
	Entity& ent = Entities[newOne.ID];

	bool created = _archetypes[id](*this, newOne);
	if (!created)
	{
		Destroy(newOne);
		
		return EntityHandle{};
	} 
	return newOne;
}

bool core::World::Destroy(EntityHandle handle)
{
	if (!Contains(handle))
		return false;

	auto& entRef = Entities[handle.ID]; 

	for (Dict<tTypeID, UniqueHandle>::Record& kv : Storages)
	{
		if (0 != (kv.Value.kMask & entRef.ComponentMask))
		{
			kv.Value.Get()->Remove(handle);
		}
	}
	entRef.ComponentMask = 0;

	Entities[handle.ID].Handle = {}; 
	_freeStack.push_back(handle.ID);

	return true;
}
 
core::EntityHandle core::World::CreateBlank(const char* name)
{ 
	int pos = Entities.size();
	if (_freeStack.size() > 0)
	{ 
		pos = _freeStack.back();
		_freeStack.pop_back(); 
	}
	else
	{
		Entities.push_back({});
	}


	Entity& ent = Entities[pos];
	ent.Handle = EntityHandle{ pos };
	if (name != nullptr)
	{
		DebugName nameStruct; nameStruct.Name = name;
		CreateComponent<DebugName>(ent, nameStruct);
	}
	return ent;
}

EntityHandle core::World::Clone(EntityHandle original)
{
	if (!Contains(original))
		return EntityHandle{};

	Entity& orig = Entities[original];
	EntityHandle newOne = CreateBlank("1");
	Entity& newEnt = Entities[newOne];

	for (auto& kv : Storages)
	{
		if (0 != (kv.Value.kMask & orig.ComponentMask))
		{
			kv.Value.Get()->Clone(original, newOne);
		}
	}
	newEnt.ComponentMask = orig.ComponentMask;

	return newOne;
}

core::World::UniqueHandle::~UniqueHandle()
{ 
	if (kDebugGuard == DebugGuard)
		delete _data; 
	else
	{  
		__debugbreak();
	}
}

core::World::UniqueHandle&  core::World::UniqueHandle::operator=(core::World::UniqueHandle&& other) noexcept
{
	if (this != &other)
	{
		_data = other._data;

		other._data = nullptr;
	}

	return *this;
}
