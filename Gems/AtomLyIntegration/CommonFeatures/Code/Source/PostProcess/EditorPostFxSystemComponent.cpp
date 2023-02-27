/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/EditorPostFxSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace AZ
{
    namespace Render
    {
        void EditorPostFxSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            EditorPostFxLayerCategoriesAsset::Reflect(context);

            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorPostFxSystemComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorPostFxSystemComponent>("Editor PostFx System", "Manages discovery of PostFx layer categories asset")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void EditorPostFxSystemComponent::Init()
        {
            AzToolsFramework::Components::EditorComponentBase::Init();
        }

        void EditorPostFxSystemComponent::Activate()
        {
            RegisterAssethandlers();
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
            AzToolsFramework::Components::EditorComponentBase::Activate();
            PostFxLayerCategoriesProviderRequestBus::Handler::BusConnect();
        }

        void EditorPostFxSystemComponent::Deactivate()
        {
            PostFxLayerCategoriesProviderRequestBus::Handler::BusDisconnect();
            AzToolsFramework::Components::EditorComponentBase::Deactivate();
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            UnregisterAssethandlers();
        }

        void EditorPostFxSystemComponent::RegisterAssethandlers()
        {
            m_postFxLayerCategoriesAsset = aznew AzFramework::GenericAssetHandler<EditorPostFxLayerCategoriesAsset>("PostFx Layer Categories", "Other", "postFxLayerCategories");
            m_postFxLayerCategoriesAsset->Register();
        }

        void EditorPostFxSystemComponent::UnregisterAssethandlers()
        {
            m_layerCategoriesAssetMap.clear();
            if (m_postFxLayerCategoriesAsset)
            {
                m_postFxLayerCategoriesAsset->Unregister();
                delete m_postFxLayerCategoriesAsset;
                m_postFxLayerCategoriesAsset = nullptr;
            }
        }

        void EditorPostFxSystemComponent::GetLayerCategories(PostFx::LayerCategoriesMap& layerCategories) const
        {
            // use layer category definitions from all postfxlayercategories asset files
            for (const auto& assetPair : m_layerCategoriesAssetMap)
            {
                const auto& asset = assetPair.second;
                if (asset.IsReady())
                {
                    const auto& priorityLayerPairs = asset.Get()->m_layerCategories;
                    layerCategories.insert(priorityLayerPairs.begin(), priorityLayerPairs.end());
                }
            }

            // insert default layer
            layerCategories[PostFx::DefaultLayerCategory] = PostFx::DefaultLayerCategoryValue;
        }

        void EditorPostFxSystemComponent::OnCatalogLoaded(const char* catalogFile)
        {
            AZ_UNUSED(catalogFile);

            // automatically register all layer categories assets
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
                nullptr,
                [this](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo) {
                    const auto assetType = azrtti_typeid<EditorPostFxLayerCategoriesAsset>();
                    if (assetInfo.m_assetType == assetType)
                    {
                        m_layerCategoriesAssetMap[assetId] = AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::PreLoad);
                        m_layerCategoriesAssetMap[assetId].QueueLoad();
                    }
                },
                nullptr);
        }

        void EditorPostFxSystemComponent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
        {
            UpdateLayerCategoriesAssetMap(assetId);
        }

        void EditorPostFxSystemComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
        {
            UpdateLayerCategoriesAssetMap(assetId);
        }

        void EditorPostFxSystemComponent::UpdateLayerCategoriesAssetMap(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);

            const auto assetType = azrtti_typeid<EditorPostFxLayerCategoriesAsset>();
            if (assetInfo.m_assetType == assetType)
            {
                m_layerCategoriesAssetMap[assetId] = AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::PreLoad);
                m_layerCategoriesAssetMap[assetId].BlockUntilLoadComplete();
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
            }
        }

        void EditorPostFxSystemComponent::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo&)
        {
            m_layerCategoriesAssetMap.erase(assetId);
        }
    }
}
