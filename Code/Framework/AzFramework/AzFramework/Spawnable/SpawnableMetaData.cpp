/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/ObjectStream.h> // Needed for ObjectStreamWriteOverrideCB and its AZ_TYPE_INFO_SPECIALIZE
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/typetraits.h>
#include <AzFramework/Spawnable/SpawnableMetaData.h>

namespace AzFramework
{
    SpawnableMetaData::SpawnableMetaData(Table table)
    {
        AZ_Assert(AZStd::is_sorted(table.begin(), table.end(),
            [](const auto& lhs, const auto& rhs)
            {
                return lhs.first < rhs.first;
            }), "The key/value table provided to SpawnableMetaData needs to be sorted by key.");
        m_keys.reserve(table.size());
        m_values.reserve(table.size());

        for (auto&& entry : table)
        {
            m_keys.push_back(entry.first);
            m_values.push_back(AZStd::move(entry.second));
        }
    }

    bool SpawnableMetaData::Get(AZStd::string_view key, bool& value) const
    {
        return GetGeneric(GetKeyHash(key), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view key, uint64_t& value) const
    {
        return GetGeneric(GetKeyHash(key), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view key, int64_t& value) const
    {
        return GetGeneric(GetKeyHash(key), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view key, double& value) const
    {
        return GetGeneric(GetKeyHash(key), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view key, AZStd::string_view& value) const
    {
        return GetGeneric(GetKeyHash(key), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view key, SpawnableMetaDataArraySize& value) const
    {
        return GetGeneric(GetKeyHash(key), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view arrayKey, uint64_t index, bool& value) const
    {
        return GetGeneric(GetKeyHash(arrayKey, index), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view arrayKey, uint64_t index, uint64_t& value) const
    {
        return GetGeneric(GetKeyHash(arrayKey, index), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view arrayKey, uint64_t index, int64_t& value) const
    {
        return GetGeneric(GetKeyHash(arrayKey, index), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view arrayKey, uint64_t index, double& value) const
    {
        return GetGeneric(GetKeyHash(arrayKey, index), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view arrayKey, uint64_t index, AZStd::string_view& value) const
    {
        return GetGeneric(GetKeyHash(arrayKey, index), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view arrayKey, SpawnableMetaDataArrayIndex index, bool& value) const
    {
        return GetGeneric(GetKeyHash(arrayKey, index), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view arrayKey, SpawnableMetaDataArrayIndex index, uint64_t& value) const
    {
        return GetGeneric(GetKeyHash(arrayKey, index), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view arrayKey, SpawnableMetaDataArrayIndex index, int64_t& value) const
    {
        return GetGeneric(GetKeyHash(arrayKey, index), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view arrayKey, SpawnableMetaDataArrayIndex index, double& value) const
    {
        return GetGeneric(GetKeyHash(arrayKey, index), value);
    }

    bool SpawnableMetaData::Get(AZStd::string_view arrayKey, SpawnableMetaDataArrayIndex index, AZStd::string_view& value) const
    {
        return GetGeneric(GetKeyHash(arrayKey, index), value);
    }

    auto SpawnableMetaData::GetType(AZStd::string_view key) const -> ValueType
    {
        return GetTypeGeneric(GetKeyHash(key));
    }

    auto SpawnableMetaData::GetType(AZStd::string_view arrayKey, uint64_t index) const -> ValueType
    {
        return GetTypeGeneric(GetKeyHash(arrayKey, index));
    }

    auto SpawnableMetaData::GetType(AZStd::string_view arrayKey, SpawnableMetaDataArrayIndex index) const -> ValueType
    {
        return GetTypeGeneric(GetKeyHash(arrayKey, index));
    }

    void SpawnableMetaData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<SpawnableMetaData>()->Version(1)
                ->Field("Keys", &SpawnableMetaData::m_keys)
                ->Field("Values", &SpawnableMetaData::m_values);
        }
    }

    auto SpawnableMetaData::GetKeyHash(AZStd::string_view key) const -> TableKey
    {
        return AZ::TypeHash64(reinterpret_cast<const uint8_t*>(key.data()), aznumeric_cast<uint64_t>(key.length()));
    }

    auto SpawnableMetaData::GetKeyHash(AZStd::string_view arrayKey, uint64_t index) const -> TableKey
    {
        return AZ::TypeHash64(
            reinterpret_cast<const uint8_t*>(arrayKey.data()), aznumeric_cast<uint64_t>(arrayKey.length()),
            aznumeric_caster(ArrayKeyRoot + index));
    }

    auto SpawnableMetaData::GetKeyHash(AZStd::string_view arrayKey, SpawnableMetaDataArrayIndex index) const -> TableKey
    {
        return GetKeyHash(arrayKey, aznumeric_cast<uint64_t>(index));
    }

    template<typename T>
    bool SpawnableMetaData::GetGeneric(AZ::HashValue64 key, T& value) const
    {
        auto it = AZStd::lower_bound(m_keys.begin(), m_keys.end(), key);
        if (it != m_keys.end() && *it == key)
        {
            size_t index = AZStd::distance(m_keys.begin(), it);
            if constexpr (AZStd::is_same_v<T, AZStd::string_view>)
            {
                if (const AZStd::string* storedValue = AZStd::get_if<AZStd::string>(&m_values[index]); storedValue != nullptr)
                {
                    value = *storedValue;
                    return true;
                }
            }
            else
            {
                if (const T* storedValue = AZStd::get_if<T>(&m_values[index]); storedValue != nullptr)
                {
                    value = *storedValue;
                    return true;
                }
            }
        }

        return false;
    }

    auto SpawnableMetaData::GetTypeGeneric(AZ::HashValue64 key) const -> ValueType
    {
        auto it = AZStd::lower_bound(m_keys.begin(), m_keys.end(), key);
        if (it != m_keys.end() && *it == key)
        {
            size_t index = AZStd::distance(m_keys.begin(), it);
            return AZStd::visit([](auto&& args) -> ValueType
            {
                using Key = AZStd::decay_t<decltype(args)>;
                     if constexpr (AZStd::is_same_v<Key, bool>)         { return ValueType::Boolean; }
                else if constexpr (AZStd::is_same_v<Key, uint64_t>)     { return ValueType::UnsignedInteger; }
                else if constexpr (AZStd::is_same_v<Key, int64_t>)      { return ValueType::SignedInteger; }
                else if constexpr (AZStd::is_same_v<Key, double>)       { return ValueType::FloatingPoint; }
                else if constexpr (AZStd::is_same_v<Key, AZStd::string>){ return ValueType::String; }
                else if constexpr (AZStd::is_same_v<Key, SpawnableMetaDataArraySize>)
                {
                    return ValueType::ArraySize;
                }
                else
                {
                    return ValueType::Unavailable;
                }
                
            }, m_values[index]);
        }
        else
        {
            return ValueType::Unavailable;
        }
    }
}; // namespace AzFramework
