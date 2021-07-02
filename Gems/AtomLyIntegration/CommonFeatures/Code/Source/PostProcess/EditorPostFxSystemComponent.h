/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerCategoriesProviderRequestBus.h>
#include <Atom/Feature/PostProcess/PostFxLayerCategoriesConstants.h>
#include <PostProcess/EditorPostFxLayerCategoriesAsset.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

namespace AZ
{
    namespace Render
    {
        class EditorPostFxSystemComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , private PostFxLayerCategoriesProviderRequestBus::Handler
            , private AzFramework::AssetCatalogEventBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(EditorPostFxSystemComponent, "{D86D2F88-ACDC-49B3-89D3-AE2EC5B8FEBC}");

            static void Reflect(AZ::ReflectContext* context);

            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // PostFxLayerCategoriesProviderRequestBus Override
            void GetLayerCategories(PostFx::LayerCategoriesMap& layerCategories) const override;

            // AssetCatalogEventBus Overrides
            void OnCatalogLoaded(const char* catalogFile) override;
            void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
            void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
            void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;
            void UpdateLayerCategoriesAssetMap(const AZ::Data::AssetId& assetId);

            // Asset handler registration
            void RegisterAssethandlers();
            void UnregisterAssethandlers();
            AzFramework::GenericAssetHandler<EditorPostFxLayerCategoriesAsset>* m_postFxLayerCategoriesAsset = nullptr;
            AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<EditorPostFxLayerCategoriesAsset>> m_layerCategoriesAssetMap;
        };
    }
}
