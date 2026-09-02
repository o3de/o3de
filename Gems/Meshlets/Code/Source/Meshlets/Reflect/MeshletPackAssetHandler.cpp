/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Meshlets/Reflect/MeshletPackAssetHandler.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace AZ::Meshlets
{
    MeshletPackAssetHandler::MeshletPackAssetHandler() = default;

    MeshletPackAssetHandler::~MeshletPackAssetHandler()
    {
        if (m_registered)
        {
            Unregister();
        }
    }

    AZ::Data::AssetPtr MeshletPackAssetHandler::CreateAsset(
        const AZ::Data::AssetId&, const AZ::Data::AssetType& type)
    {
        if (type != azrtti_typeid<MeshletPackAsset>())
        {
            return nullptr;
        }
        return aznew MeshletPackAsset();
    }

    void MeshletPackAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void MeshletPackAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& types)
    {
        types.push_back(azrtti_typeid<MeshletPackAsset>());
    }

    AZ::Data::AssetHandler::LoadResult MeshletPackAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& /*filterCB*/)
    {
        auto* packAsset = asset.GetAs<MeshletPackAsset>();
        if (!packAsset || !stream)
        {
            return AZ::Data::AssetHandler::LoadResult::Error;
        }

        // Read the entire stream into a buffer.
        const AZ::u64 size = stream->GetLength();
        AZStd::vector<AZ::u8> bytes(static_cast<size_t>(size));
        if (size > 0 && stream->Read(size, bytes.data()) != size)
        {
            return AZ::Data::AssetHandler::LoadResult::Error;
        }

        if (!packAsset->LoadFromBuffer(AZStd::move(bytes)))
        {
            return AZ::Data::AssetHandler::LoadResult::Error;
        }
        return AZ::Data::AssetHandler::LoadResult::LoadComplete;
    }

    void MeshletPackAssetHandler::Register()
    {
        if (m_registered) return;

        AZ::Data::AssetManager::Instance().RegisterHandler(this, azrtti_typeid<MeshletPackAsset>());

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::AddExtension,
            MeshletPackAsset::Extension);
        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnableCatalogForAsset,
            azrtti_typeid<MeshletPackAsset>());

        m_registered = true;
    }

    void MeshletPackAssetHandler::Unregister()
    {
        if (!m_registered) return;
        AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        m_registered = false;
    }

} // namespace AZ::Meshlets
