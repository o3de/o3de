/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/ProceduralPrefabSystemComponent.h>
#include <Prefab/Procedural/ProceduralPrefabAsset.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void ProceduralPrefabSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializationContext = azdynamic_cast<AZ::SerializeContext*>(context))
            {
                serializationContext->Class<ProceduralPrefabSystemComponent>();
            }
        }

        void ProceduralPrefabSystemComponent::Activate()
        {
            AZ::Interface<ProceduralPrefabSystemComponentInterface>::Register(this);
        }

        void ProceduralPrefabSystemComponent::Deactivate()
        {
            AZ::Interface<ProceduralPrefabSystemComponentInterface>::Unregister(this);

            m_templateToAssetLookup.clear();
        }

        AZStd::string ProceduralPrefabSystemComponent::Load(AZ::Data::AssetId assetId)
        {
            auto asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::Prefab::ProceduralPrefabAsset>(assetId, AZ::Data::Default);
            
            m_templateToAssetLookup[asset.Get()->GetTemplateId()] = asset;

            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);

            asset.BlockUntilLoadComplete();

            return assetInfo.m_relativePath;
        }
    } // namespace Prefab
} // namespace AzToolsFramework
