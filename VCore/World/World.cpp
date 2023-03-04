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
#include "World.h"

#include <VCore/Memory/Memory.h>
#include <VCore/World/World.h>

using namespace vex;

bool vex::World::destroy(EntityHandle handle)
{
    if (!contains(handle))
        return false;

    auto& comp_set = component_sets[handle.id];
    for (Dict<CompIdType, StoragePtr>::Record& kv : storages)
    {
        if (comp_set.has(kv.key, kv.value.mod_mask))
        {
            kv.value.get()->remove(handle);
        }
    }
    comp_set.bitset = {};

    ent_handles[handle.id] = {.id = 0, .gen = handle.gen++};
    free_list.push_back(handle.id);

    return true;
}
vex::EntityHandle vex::World::createBlank()
{
    EntId pos = (EntId)ent_handles.size();
    if (free_list.size() > 0)
    {
        pos = free_list.back();
        free_list.pop_back();

        auto& ent = ent_handles[pos];
        ent.id = EntId{pos};
        ent.gen++;
        return ent;
    }
    else
    {
        ent_handles.push_back({});
        component_sets.push_back({});

        ent_handles[pos] = {pos, 0}; 
        return ent_handles[pos];
    }
}
vex::EntityHandle vex::World::createBlank(const char* name)
{
    auto ent = createBlank();
    if (name != nullptr)
    {
        DebugName nameStruct;
        nameStruct.name = name;
        add<DebugName>(ent, nameStruct);
    }
    return ent;
}
vex::StoragePtr& vex::StoragePtr::operator=(vex::StoragePtr&& other) noexcept
{
    if (this != &other)
    {
        storage = other.storage;
        other.storage = nullptr;
    }

    return *this;
}
