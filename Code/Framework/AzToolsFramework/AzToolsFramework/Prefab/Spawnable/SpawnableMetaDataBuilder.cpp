/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableMetaDataBuilder.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::Add(AZStd::string_view key, bool value)
    {
        return AddGeneric(key, value);
    }

    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::Add(AZStd::string_view key, uint64_t value)
    {
        return AddGeneric(key, value);
    }

    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::Add(AZStd::string_view key, int64_t value)
    {
        return AddGeneric(key, value);
    }

    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::Add(AZStd::string_view key, double value)
    {
        return AddGeneric(key, value);
    }

    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::Add(AZStd::string_view key, AZStd::string value)
    {
        return AddGeneric(key, AZStd::move(value));
    }

    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::AppendArray(AZStd::string_view arrayKey, bool value)
    {
        return AppendArrayGeneric(arrayKey, value);
    }

    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::AppendArray(AZStd::string_view arrayKey, uint64_t value)
    {
        return AppendArrayGeneric(arrayKey, value);
    }

    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::AppendArray(AZStd::string_view arrayKey, int64_t value)
    {
        return AppendArrayGeneric(arrayKey, value);
    }

    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::AppendArray(AZStd::string_view arrayKey, double value)
    {
        return AppendArrayGeneric(arrayKey, value);
    }

    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::AppendArray(AZStd::string_view arrayKey, AZStd::string value)
    {
        return AppendArrayGeneric(arrayKey, AZStd::move(value));
    }

    bool SpawnableMetaDataBuilder::Remove(AZStd::string_view key)
    {
        auto it = m_table.find(HashKey(key));
        if (it != m_table.end())
        {
            RemoveAllEntriesIfArray(key, it);
            m_table.erase(it);
            return true;
        }
        return false;
    }

    bool SpawnableMetaDataBuilder::RemoveArrayEntry(AZStd::string_view arrayKey, uint64_t index)
    {
        return RemoveArrayEntry(arrayKey, aznumeric_cast<AzFramework::SpawnableMetaDataArrayIndex>(index));
    }

    bool SpawnableMetaDataBuilder::RemoveArrayEntry(AZStd::string_view arrayKey, AzFramework::SpawnableMetaDataArrayIndex index)
    {
        auto it = m_table.find(HashKey(arrayKey));
        if (it != m_table.end())
        {
            if (AzFramework::SpawnableMetaDataArraySize* size =
                AZStd::get_if<AzFramework::SpawnableMetaDataArraySize>(&it->second); size != nullptr)
            {
                if (index < *size)
                {
                    AZ::HashValue64 indexHash = HashArrayKey(arrayKey, index);
                    index++;
                    for (; index < *size; ++index)
                    {
                        AZ::HashValue64 nextIndexHash = HashArrayKey(arrayKey, index);
                        m_table[indexHash] = AZStd::move(m_table[nextIndexHash]);
                        indexHash = nextIndexHash;
                    }
                    [[maybe_unused]] size_t removedCount = m_table.erase(indexHash);
                    AZ_Assert(removedCount == 1, "RemoveArrayEntry did not correctly detect an edge case.");

                    (*size)--;

                    return true;
                }
            }
        }
        return false;
    }

    size_t SpawnableMetaDataBuilder::GetEntryCount() const
    {
        return m_table.size();
    }

    AzFramework::SpawnableMetaData SpawnableMetaDataBuilder::BuildMetaData() const
    {
        AzFramework::SpawnableMetaData::Table readOnlyTable;

        readOnlyTable.reserve(m_table.size());
        AZStd::transform(m_table.begin(), m_table.end(), AZStd::back_inserter(readOnlyTable),
            [](const auto& entry)
            {
                return AzFramework::SpawnableMetaData::TableEntry(entry.first, entry.second);
            });

        AZStd::sort(readOnlyTable.begin(), readOnlyTable.end(),
            [](const auto& lhs, const auto& rhs)
            {
                return lhs.first < rhs.first;
            });

        return AzFramework::SpawnableMetaData(AZStd::move(readOnlyTable));
    }

    AZ::HashValue64 SpawnableMetaDataBuilder::HashKey(AZStd::string_view key) const
    {
        return AZ::TypeHash64(reinterpret_cast<const uint8_t*>(key.data()), aznumeric_cast<uint64_t>(key.length()));
    }

    AZ::HashValue64 SpawnableMetaDataBuilder::HashArrayKey(AZStd::string_view arrayKey, uint64_t index) const
    {
        return AZ::TypeHash64(reinterpret_cast<const uint8_t*>(arrayKey.data()), aznumeric_cast<uint64_t>(arrayKey.length()),
            aznumeric_caster(AzFramework::SpawnableMetaData::ArrayKeyRoot + index));
    }

    AZ::HashValue64 SpawnableMetaDataBuilder::HashArrayKey(AZStd::string_view arrayKey, AzFramework::SpawnableMetaDataArrayIndex index) const
    {
        return HashArrayKey(arrayKey, aznumeric_cast<uint64_t>(index));
    }

    template<typename T>
    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::AddGeneric(AZStd::string_view key, T&& value)
    {
        auto keyHash = HashKey(key);
        auto it = m_table.find(keyHash);
        if (it != m_table.end())
        {
            RemoveAllEntriesIfArray(key, it);
            it->second = AZStd::forward<T>(value);
        }
        else
        {
            m_table.emplace(keyHash, AZStd::forward<T>(value));
        }
        return *this;
    }

    template<typename T>
    SpawnableMetaDataBuilder& SpawnableMetaDataBuilder::AppendArrayGeneric(AZStd::string_view arrayKey, T&& value)
    {
        auto arrayKeyHash = HashKey(arrayKey);
        auto it = m_table.find(arrayKeyHash);
        if (it != m_table.end())
        {
            if (AzFramework::SpawnableMetaDataArraySize* storedValue =
                AZStd::get_if<AzFramework::SpawnableMetaDataArraySize>(&it->second); storedValue != nullptr)
            {
                m_table[HashArrayKey(arrayKey, (*storedValue)++)] = AZStd::forward<T>(value);
            }
            else
            {
                it->second = AzFramework::SpawnableMetaDataArraySize{ 1 };
                m_table[HashArrayKey(arrayKey, 0)] = AZStd::forward<T>(value);
            }
        }
        else
        {
            m_table.emplace(arrayKeyHash, AzFramework::SpawnableMetaDataArraySize{ 1 });
            m_table[HashArrayKey(arrayKey, 0)] = AZStd::forward<T>(value);
        }
        return *this;
    }

    void SpawnableMetaDataBuilder::RemoveAllEntriesIfArray(AZStd::string_view arrayKey, Table::iterator sizeEntry)
    {
        if (AzFramework::SpawnableMetaDataArraySize* size =
            AZStd::get_if<AzFramework::SpawnableMetaDataArraySize>(&sizeEntry->second); size != nullptr)
        {
            for (AzFramework::SpawnableMetaDataArrayIndex i{ 0 }; i < *size; ++i)
            {
                [[maybe_unused]] size_t removedCount = m_table.erase(HashArrayKey(arrayKey, i));
                AZ_Assert(removedCount == 1, "RemoveArrayEntry did not correctly detect an edge case.");
            }
        }
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
