/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Math/Uuid.h>
#include <AzToolsFramework/Prefab/Spawnable/ProcesedObjectStore.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    ProcessedObjectStore::ProcessedObjectStore(AZStd::string uniqueId, AZStd::any object, SerializerFunction objectSerializer,
        AZ::Data::AssetType assetType)
        : m_uniqueId(AZStd::move(uniqueId))
        , m_object(AZStd::move(object))
        , m_objectSerializer(AZStd::move(objectSerializer))
        , m_assetType(AZStd::move(assetType))
    {
    }

    bool ProcessedObjectStore::Serialize(AZStd::vector<uint8_t>& output) const
    {
        if (m_objectSerializer)
        {
            return m_objectSerializer(output, *this);
        }
        else
        {
            return false;
        }
    }

    const AZStd::any& ProcessedObjectStore::GetObject() const
    {
        return m_object;
    }

    AZStd::any ProcessedObjectStore::ReleaseObject()
    {
        return AZStd::move(m_object);
    }

    uint32_t ProcessedObjectStore::BuildSubId() const
    {
        AZ::Uuid subIdHash = AZ::Uuid::CreateData(m_uniqueId.data(), m_uniqueId.size());
        return azlossy_caster(subIdHash.GetHash());
    }

    const AZ::Data::AssetType& ProcessedObjectStore::GetAssetType() const
    {
        return m_assetType;
    }

    const AZStd::string& ProcessedObjectStore::GetId() const
    {
        return m_uniqueId;
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
