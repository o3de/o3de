/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/Spawnable/SpawnableMetaData.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class SpawnableMetaDataBuilder final
    {
    public:
        SpawnableMetaDataBuilder& Add(AZStd::string_view key, bool value);
        SpawnableMetaDataBuilder& Add(AZStd::string_view key, uint64_t value);
        SpawnableMetaDataBuilder& Add(AZStd::string_view key, int64_t value);
        SpawnableMetaDataBuilder& Add(AZStd::string_view key, double value);
        SpawnableMetaDataBuilder& Add(AZStd::string_view key, AZStd::string value);

        SpawnableMetaDataBuilder& AppendArray(AZStd::string_view arrayKey, bool value);
        SpawnableMetaDataBuilder& AppendArray(AZStd::string_view arrayKey, uint64_t value);
        SpawnableMetaDataBuilder& AppendArray(AZStd::string_view arrayKey, int64_t value);
        SpawnableMetaDataBuilder& AppendArray(AZStd::string_view arrayKey, double value);
        SpawnableMetaDataBuilder& AppendArray(AZStd::string_view arrayKey, AZStd::string value);

        bool Remove(AZStd::string_view key);
        bool RemoveArrayEntry(AZStd::string_view arrayKey, uint64_t index);
        bool RemoveArrayEntry(AZStd::string_view arrayKey, AzFramework::SpawnableMetaDataArrayIndex index);

        size_t GetEntryCount() const;

        AzFramework::SpawnableMetaData BuildMetaData() const;

    private:
        using Table = AZStd::unordered_map<AZ::HashValue64, AzFramework::SpawnableMetaData::TableValue>;

        AZ::HashValue64 HashKey(AZStd::string_view key) const;
        AZ::HashValue64 HashArrayKey(AZStd::string_view arrayKey, uint64_t index) const;
        AZ::HashValue64 HashArrayKey(AZStd::string_view arrayKey, AzFramework::SpawnableMetaDataArrayIndex index) const;

        template<typename T>
        SpawnableMetaDataBuilder& AddGeneric(AZStd::string_view key, T&& value);

        template<typename T>
        SpawnableMetaDataBuilder& AppendArrayGeneric(AZStd::string_view arrayKey, T&& value);

        void RemoveAllEntriesIfArray(AZStd::string_view arrayKey, Table::iterator sizeEntry);

        Table m_table;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
