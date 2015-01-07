#include "interned_string.hh"

namespace Kakoune
{

InternedString StringRegistry::acquire(StringView str)
{
    auto it = m_slot_map.find(str);
    if (it == m_slot_map.end())
    {
        size_t slot;
        if (not m_free_slots.empty())
        {
            slot = m_free_slots.back();
            m_free_slots.pop_back();
            kak_assert(m_storage[slot].refcount == 0);
            m_storage[slot] = DataAndRefCount{{str.begin(), str.end()}, 1};
        }
        else
        {
            slot = m_storage.size();
            m_storage.push_back({{str.begin(), str.end()}, 1});
        }
        // Create a new string view that point to the storage data
        StringView storage_view{m_storage[slot].data.data(), (int)m_storage[slot].data.size()};
        m_slot_map[storage_view] = slot;

        return InternedString{storage_view, slot};
    }

    size_t slot = it->second;
    auto& data = m_storage[slot];
    ++data.refcount;
    return {{data.data.data(), (int)data.data.size()}, slot};
}

void StringRegistry::acquire(size_t slot)
{
    kak_assert(slot < m_storage.size());
    kak_assert(m_storage[slot].refcount > 0);
    ++m_storage[slot].refcount;
}

void StringRegistry::release(size_t slot) noexcept
{
    kak_assert(m_storage[slot].refcount > 0);
    if (--m_storage[slot].refcount == 0)
    {
        m_free_slots.push_back(slot);
        auto& data = m_storage[slot].data;
        auto it = m_slot_map.find(StringView{data.data(), (int)data.size()});
        kak_assert(it != m_slot_map.end());
        m_slot_map.erase(it);
        data = Vector<char, MemoryDomain::InternedString>{};
    }
}

}
