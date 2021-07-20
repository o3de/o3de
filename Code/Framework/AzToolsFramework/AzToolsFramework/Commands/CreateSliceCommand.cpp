/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzToolsFramework_precompiled.h"
#include "CreateSliceCommand.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/Utils.h>

#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
namespace AzToolsFramework
{
    CreateSliceCommand::CreateSliceCommand(const AZStd::string& friendlyName)
        : BaseSliceCommand(friendlyName)
    {
    }

    void CreateSliceCommand::Capture(const AZ::Data::Asset<AZ::SliceAsset>& tempSliceAsset,
        const AZStd::string& fullSliceAssetPath,
        const AZ::SliceComponent::EntityIdToEntityIdMap& liveToAssetMap)
    {
        if (!tempSliceAsset.Get())
        {
            AZ_Error("CreateSliceCommand::Capture",
                false,
                "Invalid SliceAsset passed in. Unable to capture undo/redo state");

            return;
        }

        if (liveToAssetMap.empty())
        {
            AZ_Warning("CreateSliceCommand::Capture",
                false,
                "Empty liveToAsset EntityId map passed in. Nothing to undo/redo");

            return;
        }

        m_fullSliceAssetPath = fullSliceAssetPath;
        m_liveToAssetMap = liveToAssetMap;

        // Get the sourceUUID of our newly created slice so that we can build its asset ID without having to wait for it to be processed
        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;
        bool foundSourceInfo = false;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundSourceInfo,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
            m_fullSliceAssetPath.c_str(), assetInfo, watchFolder);

        if (!foundSourceInfo)
        {
            AZ_Error("CreateSliceCommand::Capture",
                false,
                "Failed to acquire asset source info for asset located at local path: %s. Unable to capture undo/redo state",
                m_fullSliceAssetPath.c_str());

            m_liveToAssetMap.clear();
            return;
        }

        // Convert retrieved assetId to have the appropriate subID for a static (non-dynamic) slice asset
        m_sliceAssetId = AZ::Data::AssetId(assetInfo.m_assetId.m_guid, AZ::SliceAsset::GetAssetSubId());

        // Save a copy of our tempSliceAsset
        AZ::SliceAsset* tempSliceAssetData = tempSliceAsset.Get();
        AZ::Entity* tempSliceAssetEntity = tempSliceAssetData->GetEntity();
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> assetStream(&m_sliceAssetBuffer);
        bool saveSuccess = AZ::Utils::SaveObjectToStream(assetStream, AZ::DataStream::ST_BINARY, tempSliceAssetEntity);

        if (!saveSuccess)
        {
            AZ_Error("CreateSliceCommand::Capture",
                false,
                "Failed to cache provided slice asset specified at local path: %s. Unable to capture undo/redo state",
                m_fullSliceAssetPath.c_str());

            m_liveToAssetMap.clear();
            return;
        }

        // Capture restore info for all the live entities in our map
        for (const AZStd::pair<AZ::EntityId, AZ::EntityId>& liveToAssetIdPair : m_liveToAssetMap)
        {
            const AZ::EntityId& liveEntityId = liveToAssetIdPair.first;

            bool restoreCaptured = BaseSliceCommand::CaptureRestoreInfoForUndo(liveEntityId);

            if(!restoreCaptured)
            {
                m_liveToAssetMap.clear();
                m_entityUndoRestoreInfoArray.clear();

                return;
            }
        }

        // Remove the dirty flag on these entities
        // Otherwise an additional entity restore will be scheduled on these entities
        for (const AZStd::pair<AZ::EntityId, AZ::EntityId>& liveToAssetId : m_liveToAssetMap)
        {
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::RemoveDirtyEntity, liveToAssetId.first);
        }
    }

    void CreateSliceCommand::Undo()
    {
        BaseSliceCommand::RestoreEntities(m_entityUndoRestoreInfoArray);
    }

    void CreateSliceCommand::Redo()
    {
        // Load our slice asset in memory
        AZ::Data::Asset<AZ::SliceAsset> sliceAsset = PreloadSliceAsset();

        // Move all entities marked in m_liveToAssetMap into a new instance of our sliceAsset
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceResult;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(sliceInstanceResult,
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::PromoteEditorEntitiesIntoSlice,
            sliceAsset, m_liveToAssetMap);
    }

    AZ::Data::Asset<AZ::SliceAsset> CreateSliceCommand::PreloadSliceAsset()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("CreateSliceCommand::PreloadSliceAsset",
                false,
                "Failed to retrieve serialize context. Unable to proceed with loading cached slice asset for redo");

            return AZ::Data::Asset<AZ::SliceAsset>();
        }

        // Load our sliceAsset back in
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> sliceAssetStream(&m_sliceAssetBuffer);

        // Load our cached Asset Entity from stream and store it in a unique_ptr. If we early out due to an error it will clean up the loose entity
        AZStd::unique_ptr<AZ::Entity> sliceAssetEntity(AZ::Utils::LoadObjectFromStream<AZ::Entity>(sliceAssetStream, serializeContext));

        if (!sliceAssetEntity)
        {
            AZ_Assert(false,
                "CreateSliceCommand::PreloadSliceAsset could not load cached slice asset from stream. Unable to proceed with loading cached asset for redo");

            return AZ::Data::Asset<AZ::SliceAsset>();
        }

        AZ::Data::Asset<AZ::SliceAsset> sliceAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::SliceAsset>(m_sliceAssetId, AZ::Data::AssetLoadBehavior::Default);

        // No Asset should exist under this AssetID. If one does we cannot continue
        if (sliceAsset.GetStatus() != AZ::Data::AssetData::AssetStatus::NotLoaded)
        {
            AZ_Error("CreateSliceCommand::PreloadSliceAsset",
                false,
                "Asset already registered with AssetID: %s. Unable to proceed with loading cached asset for redo",
                m_sliceAssetId.ToString<AZStd::string>().c_str());

            return AZ::Data::Asset<AZ::SliceAsset>();
        }

        AZ::SliceAsset* sliceAssetData = sliceAsset.Get();

        // Confirm GetAsset was successful
        if (!sliceAssetData)
        {
            AZ_Error("CreateSliceCommand::PreloadSliceAsset",
                false,
                "Failed to get slice asset from Asset ID: %s via the AssetManager. Unable to generate initial slice instance during Create Slice",
                m_sliceAssetId.ToString<AZStd::string>().c_str());

            return AZ::Data::Asset<AZ::SliceAsset>();
        }

        // Set the new asset's data to be the data we cached in m_sliceAssetBuffer
        sliceAssetData->SetData(sliceAssetEntity.get(), sliceAssetEntity->FindComponent<AZ::SliceComponent>());

        // Validate that the entity and component of sliceAssetData were successfully set
        if (sliceAssetData->GetStatus() != AZ::Data::AssetData::AssetStatus::Ready)
        {
            AZ_Error("CreateSliceCommand::PreloadSliceAsset",
                false,
                "Failed to load valid slice data during initial creation of slice asset with AssetID: %s. Unable to generate initial slice instance during Create Slice",
                m_sliceAssetId.ToString<AZStd::string>().c_str());

            return AZ::Data::Asset<AZ::SliceAsset>();
        }

        // Safe to release our Entity to the asset
        sliceAssetEntity.release();

        // Finalize configuring the slice asset
        sliceAssetData->GetComponent()->SetMyAsset(sliceAssetData);
        sliceAssetData->GetComponent()->ListenForAssetChanges();

        // Update the asset's hint to be its relative asset path
        bool relativePathFound = false;
        AZStd::string assetPathRelative;
        AssetSystemRequestBus::BroadcastResult(relativePathFound, &AssetSystemRequestBus::Events::GetRelativeProductPathFromFullSourceOrProductPath, m_fullSliceAssetPath, assetPathRelative);
        AZStd::to_lower(assetPathRelative.begin(), assetPathRelative.end());

        sliceAsset.SetHint(assetPathRelative);

        // We've finalized loading this asset from memory
        // Ignore the following reload from AssetCatalog since it will attempt to overwrite our asset with what's on disk
        // Which is an unnecessary operation as SliceTransaction serialized the contents of transactionAsset to disk
        sliceAssetData->SetIgnoreNextAutoReload(true);

        return sliceAsset;
    }
}
