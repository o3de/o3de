/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/AssetCatalogBus.h>
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
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        void ProceduralPrefabSystemComponent::Deactivate()
        {
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            AZ::Interface<ProceduralPrefabSystemComponentInterface>::Unregister(this);

            m_templateToAssetLookup.clear();
        }

        void ProceduralPrefabSystemComponent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
        {
            auto itr = m_templateToAssetLookup.find(assetId);

            if (itr != m_templateToAssetLookup.end())
            {
                auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

                if (!prefabSystemComponentInterface)
                {
                    
                }

                AZStd::string assetPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, assetId);

                auto readResult = AZ::Utils::ReadFile(assetPath, AZStd::numeric_limits<size_t>::max());
                if (!readResult.IsSuccess())
                {
                    AZ_Error(
                        "Prefab", false,
                        "PrefabLoader::LoadTemplate - Failed to load Prefab file from '%.*s'."
                        "Error message: '%s'",
                        AZ_STRING_ARG(assetPath), readResult.GetError().c_str());
                }

                AZ::Outcome<PrefabDom, AZStd::string> readPrefabFileResult = AZ::JsonSerializationUtils::ReadJsonString(readResult.TakeValue());
                if (!readPrefabFileResult.IsSuccess())
                {
                    AZ_Error(
                        "Prefab", false,
                        "PrefabLoader::LoadTemplate - Failed to load Prefab file from '%.*s'."
                        "Error message: '%s'",
                        AZ_STRING_ARG(assetPath), readPrefabFileResult.GetError().c_str());
                    
                }

                prefabSystemComponentInterface->UpdatePrefabTemplate(itr->second, readPrefabFileResult.TakeValue());
            }
        }

        //void ProceduralPrefabSystemComponent::OnAssetReloaded([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> assetData)
        //{
        //    auto assetItr = m_templateToAssetLookup.find(assetData.GetId());

        //    if (assetItr != m_templateToAssetLookup.end())
        //    {
        //        [[maybe_unused]] AzToolsFramework::Prefab::TemplateId templateId =
        //            assetItr->second.GetAs<AZ::Prefab::ProceduralPrefabAsset>()->GetTemplateId();

        //        assetItr->second = assetData;
        //    }
        //}

        void ProceduralPrefabSystemComponent::Track(const AZStd::string& prefabFilePath, TemplateId templateId)
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, prefabFilePath.c_str(),
                azrtti_typeid<AZ::Prefab::ProceduralPrefabAsset>(), false);

            if (assetId.IsValid())
            {
                m_templateToAssetLookup[assetId] = templateId;
            }
        }

        //AZStd::string ProceduralPrefabSystemComponent::Load(AZ::Data::AssetId assetId)
        //{
        //    auto asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::Prefab::ProceduralPrefabAsset>(assetId, AZ::Data::Default);

        //    AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        //    m_templateToAssetLookup[assetId] = asset;

        //    AZ::Data::AssetInfo assetInfo;
        //    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);

        //    asset.BlockUntilLoadComplete();

        //    return assetInfo.m_relativePath;
        //}
    } // namespace Prefab
} // namespace AzToolsFramework
