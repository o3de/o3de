/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/IdUtils.h>
#include <Editor/Assets/ScriptCanvasAssetInstance.h>

namespace ScriptCanvasEditor
{
    ScriptCanvasAssetInstance::~ScriptCanvasAssetInstance()
    {
    }

    void ScriptCanvasAssetInstance::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptCanvasAssetInstance>()
                // TODO: This results in more data being saved per instance, but is needed to make Id remapping transparent.
                // Allow a way to enumerate the elements for Id Remapping, while disallowing serialization
                ->Field("m_data", &ScriptCanvasAssetInstance::m_scriptCanvasData) 
                ->Field("m_assetRef", &ScriptCanvasAssetInstance::m_assetRef)
                ->Field("m_entityInstanceMap", &ScriptCanvasAssetInstance::m_baseToInstanceMap)
                ->Field("m_dataFlags", &ScriptCanvasAssetInstance::m_entityToDataFlags)
                ->Field("m_dataPatch", &ScriptCanvasAssetInstance::m_dataPatch)
                ;
        }
    }

    ScriptCanvasAssetReference& ScriptCanvasAssetInstance::GetReference()
    {
        return m_assetRef;
    }

    const ScriptCanvasAssetReference& ScriptCanvasAssetInstance::GetReference() const
    {
        return m_assetRef;
    }

    ScriptCanvas::ScriptCanvasData& ScriptCanvasAssetInstance::GetScriptCanvasData()
    {
        return m_scriptCanvasData;
    }

    const ScriptCanvas::ScriptCanvasData& ScriptCanvasAssetInstance::GetScriptCanvasData() const
    {
        return m_scriptCanvasData;
    }

    void ScriptCanvasAssetInstance::ComputeDataPatch()
    {
        if (!m_assetRef.GetAsset().IsReady())
        {
            return;
        }

        ScriptCanvas::ScriptCanvasData& baseData = m_assetRef.GetAsset().Get()->GetScriptCanvasData();

        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        decltype(m_baseToInstanceMap) entityIdReverseLookUp;
        for (const auto& baseToInstancePair : m_baseToInstanceMap)
        {
            entityIdReverseLookUp.emplace(baseToInstancePair.second, baseToInstancePair.first);
        }

        // remap entity ids to the "original"
        AZ::IdUtils::Remapper<AZ::EntityId>::ReplaceIdsAndIdRefs(&m_scriptCanvasData, [&entityIdReverseLookUp](AZ::EntityId sourceId, [[maybe_unused]] bool isEntityId, const AZ::IdUtils::Remapper<AZ::EntityId>::IdGenerator&) -> AZ::EntityId
        {
            auto findIt = entityIdReverseLookUp.find(sourceId);
            return findIt != entityIdReverseLookUp.end() ? findIt->second : sourceId;
        }, serializeContext);

        // compute the delta (what we changed from the base slice)
        m_dataPatch.Create(&baseData, &m_scriptCanvasData, AZ::DataPatch::FlagsMap(), GetDataFlagsForPatching(), serializeContext);

        // remap entity ids back to the "instance onces"
        AZ::IdUtils::Remapper<AZ::EntityId>::ReplaceIdsAndIdRefs(&m_scriptCanvasData, [this](AZ::EntityId sourceId, [[maybe_unused]] bool isEntityId, const AZ::IdUtils::Remapper<AZ::EntityId>::IdGenerator&) -> AZ::EntityId {
            auto findIt = m_baseToInstanceMap.find(sourceId);
            return findIt != m_baseToInstanceMap.end() ? findIt->second : sourceId;
        }, serializeContext);

    }

    void ScriptCanvasAssetInstance::ApplyDataPatch()
    {
        if (!m_assetRef.GetAsset().IsReady())
        {
            return;
        }

        ScriptCanvas::ScriptCanvasData& baseData = m_assetRef.GetAsset().Get()->GetScriptCanvasData();

        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        // An empty map indicates its a fresh instance (i.e. has never be instantiated and then serialized).
        if (m_baseToInstanceMap.empty())
        {
            // Generate new Ids and populate the map.
            AZ_Assert(!m_dataPatch.IsValid(), "Data patch is valid for scene slice instance, but base scene to instantiated scene Id map is not!");
            serializeContext->CloneObjectInplace(m_scriptCanvasData, &baseData);
            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&m_scriptCanvasData, m_baseToInstanceMap);
        }
        else
        {
            // Clone entities while applying any data patches.
            AZ_Assert(m_dataPatch.IsValid(), "Data patch is not valid for existing scene slice instance!");
            ScriptCanvas::ScriptCanvasData* patchedSceneData = m_dataPatch.Apply(&baseData, serializeContext);

            // Remap Ids & references.
            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(patchedSceneData, m_baseToInstanceMap);
            serializeContext->CloneObjectInplace(m_scriptCanvasData, patchedSceneData);
        }
    }

    AZ::DataPatch::FlagsMap ScriptCanvasAssetInstance::GetDataFlagsForPatching() const
    {
        // Collect all entities' data flags
        AZ::DataPatch::FlagsMap dataFlags;

        for (auto& baseIdInstanceIdPair : m_baseToInstanceMap)
        {
            // Make the addressing relative to InstantiatedContainer (m_dataFlags stores flags relative to the individual entity)
            AZ::DataPatch::AddressType addressPrefix;
            addressPrefix.push_back(AZ_CRC("Entities", 0x50ec64e5));
            addressPrefix.push_back(static_cast<AZ::u64>(baseIdInstanceIdPair.first));

            auto findIt = m_entityToDataFlags.find(baseIdInstanceIdPair.second);
            if (findIt != m_entityToDataFlags.end())
            {
                for (auto& addressDataFlagsPair : findIt->second)
                {
                    const AZ::DataPatch::AddressType& originalAddress = addressDataFlagsPair.first;

                    AZ::DataPatch::AddressType prefixedAddress;
                    prefixedAddress.reserve(addressPrefix.size() + originalAddress.size());
                    prefixedAddress.insert(prefixedAddress.end(), addressPrefix.begin(), addressPrefix.end());
                    prefixedAddress.insert(prefixedAddress.end(), originalAddress.begin(), originalAddress.end());

                    dataFlags.emplace(AZStd::move(prefixedAddress), addressDataFlagsPair.second);
                }
            }
        }

        return dataFlags;
    }
}
