/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/MeshInstanceList.h>

namespace AZ
{
    namespace Render
    {
        template<class Key, class Data>
        uint32_t SlotMap<Key, Data>::Add(Key key)
        {
            uint32_t indirectionIndex = 0;
            typename DataMap::iterator it = m_dataMap.find(key);
            if (it == m_dataMap.end())
            {
                // key not found, add it to the data list and the indirection list
                m_data.push_back(Data{});
                uint32_t dataIndex = aznumeric_cast<uint32_t>(m_data.size()) - 1;

                indirectionIndex = m_indirectionList.AddEntry(dataIndex);

                // add the data map entry containing the true index, indirection index, and reference count
                IndexMapEntry entry;
                entry.m_index = dataIndex;
                entry.m_indirectionIndex = indirectionIndex;
                entry.m_count = 1;
                m_dataMap.insert({ key, entry });

                // Keep track of any new entries, since the actual data vector is uninitialized
                m_newEntryList.push_back(indirectionIndex);
            }
            else
            {
                // data is already known, update the reference count and return the indirection index
                it->second.m_count++;
                indirectionIndex = it->second.m_indirectionIndex;
            }

            return indirectionIndex;
        }

        template<class Key, class Data>
        void SlotMap<Key, Data>::Remove(Key key)
        {
            typename DataMap::iterator it = m_dataMap.find(key);
            AZ_Assert(it != m_dataMap.end(), "Unable to find key in the DataMap");

            // decrement reference count
            it->second.m_count--;

            // if the reference count is zero then remove the entry from both the map and the main data list
            if (it->second.m_count == 0)
            {
                uint32_t dataIndex = it->second.m_index;
                uint32_t indirectionIndex = it->second.m_indirectionIndex;
                uint32_t lastDataIndex = static_cast<uint32_t>(m_data.size() - 1);

                // Since this is being removed, make sure it's no longer in the list of uninitialized data
                m_newEntryList.erase(AZStd::remove(m_newEntryList.begin(), m_newEntryList.end(), indirectionIndex));

                if (dataIndex < lastDataIndex)
                {
                    // the key we're removing is in the middle of the list - swap the last entry to this position
                    // this only works performantly if the key is also the data entry, so we have a direct lookup of last data entry->data map entry
                    bool found = false;
                    for (auto& [currentKey, currentValue] : m_dataMap)
                    {
                        if (currentValue.m_index == lastDataIndex)
                        {
                            m_data[dataIndex] = m_data.back();

                            // update the swapped entry with its new index in the resource list
                            currentValue.m_index = dataIndex;

                            // update the indirection vector entry of the swapped entry to point to the new position
                            // Note: any indirection indices returned by AddResource for other resources remain stable, this just updates
                            // the indirection entry
                            m_indirectionList.SetEntry(currentValue.m_indirectionIndex, dataIndex);
                            found = true;
                            break;
                        }
                    }
                    
                    AZ_Assert(found || m_dataMap.empty(), "Unable to find the last data entry in the DataMap");                                  
                }

                // cache the indirection index so that its okay to erase the iterator
                uint32_t cachedIndirectionIndex = it->second.m_indirectionIndex;

                // remove the last entry from the resource list
                m_data.pop_back();

                // remove the entry from the resource map
                m_dataMap.erase(it);

                // remove the entry from the indirection list
                m_indirectionList.RemoveEntry(cachedIndirectionIndex);
            }
        }

        template<class Key, class Data>
        Data& SlotMap<Key, Data>::operator[](uint32_t index)
        {
            return m_data[m_indirectionList[index]];
        }

        template<class Key, class Data>
        const Data& SlotMap<Key, Data>::operator[](uint32_t index) const
        {
            return m_data[m_indirectionList[index]];
        }

        template<class Key, class Data>
        void SlotMap<Key, Data>::Reset()
        {
            m_data.clear();
            m_dataMap.clear();
            m_indirectionList.Reset();
        }
    }
}
