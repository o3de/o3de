/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSurfaceDataSystemComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace SurfaceData
{
    namespace Details
    {
        AzFramework::GenericAssetHandler<EditorSurfaceTagListAsset>* s_surfaceTagListAssetHandler = nullptr;

        void RegisterAssethandlers()
        {
            s_surfaceTagListAssetHandler = aznew AzFramework::GenericAssetHandler<EditorSurfaceTagListAsset>("Surface Tag Name List", "Other", "surfaceTagNameList");
            s_surfaceTagListAssetHandler->Register();
        }

        void UnregisterAssethandlers()
        {
            if (s_surfaceTagListAssetHandler)
            {
                s_surfaceTagListAssetHandler->Unregister();
                delete s_surfaceTagListAssetHandler;
                s_surfaceTagListAssetHandler = nullptr;
            }
        }
    }

    void EditorSurfaceDataSystemConfig::Reflect(AZ::ReflectContext* context)
    {
        EditorSurfaceTagListAsset::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorSurfaceDataSystemConfig, AZ::ComponentConfig>()
                ->Version(0)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<EditorSurfaceDataSystemConfig>(
                    "Editor Surface Data System Config", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void EditorSurfaceDataSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorSurfaceDataSystemConfig::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorSurfaceDataSystemComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(0)
                ->Field("Configuration", &EditorSurfaceDataSystemComponent::m_configuration)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorSurfaceDataSystemComponent>("Editor Surface Data System", "Manages discovery and registration of surface tag list assets")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorSurfaceDataSystemComponent::m_configuration, "Configuration", "")
                    ;
            }
        }
    }

    void EditorSurfaceDataSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SurfaceDataTagProviderService"));
    }

    void EditorSurfaceDataSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("SurfaceDataTagProviderService"));
    }

    void EditorSurfaceDataSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("SurfaceDataSystemService"));
    }

    void EditorSurfaceDataSystemComponent::Init()
    {
        AzToolsFramework::Components::EditorComponentBase::Init();
    }

    void EditorSurfaceDataSystemComponent::Activate()
    {
        Details::RegisterAssethandlers();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AzToolsFramework::Components::EditorComponentBase::Activate();
        SurfaceDataTagProviderRequestBus::Handler::BusConnect();
    }

    void EditorSurfaceDataSystemComponent::Deactivate()
    {
        m_surfaceTagNameAssets.clear();

        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        SurfaceDataTagProviderRequestBus::Handler::BusDisconnect();
        Details::UnregisterAssethandlers();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }

    void EditorSurfaceDataSystemComponent::GetRegisteredSurfaceTagNames(SurfaceTagNameSet& masks) const
    {
        masks.insert(Constants::s_unassignedTagName);

        for (const auto& assetPair : m_surfaceTagNameAssets)
        {
            const auto& asset = assetPair.second;
            if (asset.IsReady())
            {
                const auto& tags = asset.Get()->m_surfaceTagNames;
                masks.insert(tags.begin(), tags.end());
            }
        }
    }

    void EditorSurfaceDataSystemComponent::OnCatalogLoaded(const char* /*catalogFile*/)
    {
        //automatically register all existing surface tag list assets at Editor startup

        AZStd::vector<AZ::Data::AssetId> surfaceTagAssetIds;

        // First run through all the assets and gather up the asset IDs for all surface tag list assets
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
            nullptr,
            [&surfaceTagAssetIds](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo) {
                const auto assetType = azrtti_typeid<EditorSurfaceTagListAsset>();
                if (assetInfo.m_assetType == assetType)
                {
                    surfaceTagAssetIds.emplace_back(assetId);
                }
            },
            nullptr);

        // Next, trigger all the loads.  This is done outside of EnumerateAssets to ensure that we don't have any deadlocks caused by
        // lock inversion.  If this thread locks AssetCatalogRequestBus mutex with EnumerateAssets, then locks m_assetMutex in
        // AssetManager::FindOrCreateAsset, it's possible for those locks to get locked in reverse on a loading thread, causing a deadlock.
        for (auto& assetId : surfaceTagAssetIds)
        {
            LoadAsset(assetId);
        }
    }

    void EditorSurfaceDataSystemComponent::LoadAsset(const AZ::Data::AssetId& assetId)
    {
        m_surfaceTagNameAssets[assetId] = AZ::Data::AssetManager::Instance().GetAsset(
            assetId, azrtti_typeid<EditorSurfaceTagListAsset>(), AZ::Data::AssetLoadBehavior::Default);

        // Connect to the bus for this asset so we can monitor for both OnAssetReady and OnAssetReloaded events
        AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
    }

    void EditorSurfaceDataSystemComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);

        if (assetInfo.m_assetType == azrtti_typeid<EditorSurfaceTagListAsset>())
        {
            // A new Surface Tag asset was added, so load it.
            LoadAsset(assetId);
        }
    }

    void EditorSurfaceDataSystemComponent::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo)
    {
        if (assetInfo.m_assetType == azrtti_typeid<EditorSurfaceTagListAsset>())
        {
            // A Surface Tag asset was removed, so stop listening for it and remove it from our set of loaded assets.
            // Note: This case should never really happen in practice - we're keeping the asset loaded, so the file will remain
            // locked while the Editor is running and shouldn't be able to be deleted.
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetId);
            m_surfaceTagNameAssets.erase(assetId);
        }
    }

    void EditorSurfaceDataSystemComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AddAsset(asset);
    }

    void EditorSurfaceDataSystemComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AddAsset(asset);
    }

    void EditorSurfaceDataSystemComponent::AddAsset(AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        if (asset.GetType() == azrtti_typeid<EditorSurfaceTagListAsset>())
        {
            m_surfaceTagNameAssets[asset.GetId()] = asset;
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
        }
    }
}
