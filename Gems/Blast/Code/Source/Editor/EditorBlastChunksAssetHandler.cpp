/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Editor/EditorBlastMeshDataComponent.h>
#include <Editor/EditorBlastChunksAssetHandler.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace Blast
{
    //
    // EditorBlastChunksAssetHandler
    //

    EditorBlastChunksAssetHandler::~EditorBlastChunksAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr EditorBlastChunksAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        if (type != GetAssetType())
        {
            AZ_Error("Blast", type == GetAssetType(), "Invalid asset type! We only handle 'BlastChunksAsset'");
            return {};
        }

        if (!CanHandleAsset(id))
        {
            return nullptr;
        }

        return aznew BlastChunksAsset;
    }

    AZ::Data::AssetHandler::LoadResult EditorBlastChunksAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        BlastChunksAsset* blastChunksAsset = asset.GetAs<BlastChunksAsset>();
        AZ_Error("blast", blastChunksAsset,
            "This should be a BlastChunksAsset type, as this is the only type we process!");
        if (!blastChunksAsset)
        {
            return LoadResult::Error;
        }

        // get all products from the source scene asset
        bool found = false;
        AZStd::vector<AZ::Data::AssetInfo> productsAssetInfo;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            found,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetsProducedBySourceUUID,
            asset.Get()->GetId().m_guid,
            productsAssetInfo);

        if (!found)
        {
            AZ_Error("blast",
                found,
                "Could not find asset models produced by source asset ID %s, verify the output product model assets.",
                asset.Get()->GetId().m_guid.ToString<AZStd::string>().c_str());
            return LoadResult::Error;
        }

        // find all model assets
        AZStd::vector<AZ::Data::AssetId> modelAssetIdList;
        for (const AZ::Data::AssetInfo& assetInfo : productsAssetInfo)
        {
            if (azrtti_typeid<AZ::RPI::ModelAsset>() == assetInfo.m_assetType)
            {
                modelAssetIdList.push_back(assetInfo.m_assetId);
            }
        }
        blastChunksAsset->SetModelAssetIds(modelAssetIdList);

        return LoadResult::LoadComplete;
    }

    void EditorBlastChunksAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void EditorBlastChunksAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(azrtti_typeid<BlastChunksAsset>());
    }

    void EditorBlastChunksAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, azrtti_typeid<BlastChunksAsset>());
        AZ::AssetTypeInfoBus::Handler::BusConnect(azrtti_typeid<BlastChunksAsset>());
    }

    void EditorBlastChunksAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(azrtti_typeid<BlastChunksAsset>());
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType EditorBlastChunksAssetHandler::GetAssetType() const
    {
        return azrtti_typeid<BlastChunksAsset>();
    }

    const char* EditorBlastChunksAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Blast Chunks Asset";
    }

    const char* EditorBlastChunksAssetHandler::GetGroup() const
    {
        return "Blast";
    }

    const char* EditorBlastChunksAssetHandler::GetBrowserIcon() const
    {
        return "Icons/Components/Box.png";
    }

    void EditorBlastChunksAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back("blast_chunks");
    }

} // namespace Blast
