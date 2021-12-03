/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Utils/Utils.h>
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

            AZStd::scoped_lock lock(m_lookupMutex);
            m_assetIdToTemplateLookup.clear();
        }

        void ProceduralPrefabSystemComponent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
        {
            TemplateId templateId = InvalidTemplateId;

            {
                AZStd::scoped_lock lock(m_lookupMutex);
                auto itr = m_assetIdToTemplateLookup.find(assetId);

                if (itr != m_assetIdToTemplateLookup.end())
                {
                    templateId = itr->second;
                }
            }

            if (templateId != InvalidTemplateId)
            {
                auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

                if (!prefabSystemComponentInterface)
                {
                    AZ_Error("Prefab", false, "Failed to get PrefabSystemComponentInterface");
                    return;
                }

                AZStd::string assetPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, assetId);

                auto readResult = AZ::Utils::ReadFile(assetPath, AZStd::numeric_limits<size_t>::max());
                if (!readResult.IsSuccess())
                {
                    AZ_Error(
                        "Prefab", false,
                        "ProceduralPrefabSystemComponent::OnCatalogAssetChanged - Failed to read Prefab file from '%.*s'."
                        "Error message: '%s'",
                        AZ_STRING_ARG(assetPath), readResult.GetError().c_str());
                    return;
                }

                AZ::Outcome<PrefabDom, AZStd::string> readPrefabFileResult = AZ::JsonSerializationUtils::ReadJsonString(readResult.TakeValue());
                if (!readPrefabFileResult.IsSuccess())
                {
                    AZ_Error(
                        "Prefab", false,
                        "ProceduralPrefabSystemComponent::OnCatalogAssetChanged - Failed to read Prefab json from '%.*s'."
                        "Error message: '%s'",
                        AZ_STRING_ARG(assetPath), readPrefabFileResult.GetError().c_str());
                    return;
                }

                prefabSystemComponentInterface->UpdatePrefabTemplate(templateId, readPrefabFileResult.TakeValue());
            }
        }

        void ProceduralPrefabSystemComponent::OnTemplateRemoved(TemplateId removedTemplateId)
        {
            AZStd::scoped_lock lock(m_lookupMutex);

            for (const auto& [assetId, templateId] : m_assetIdToTemplateLookup)
            {
                if (templateId == removedTemplateId)
                {
                    m_assetIdToTemplateLookup.erase(assetId);
                    break;
                }
            }
        }

        void ProceduralPrefabSystemComponent::OnAllTemplatesRemoved()
        {
            AZStd::scoped_lock lock(m_lookupMutex);

            m_assetIdToTemplateLookup.clear();
        }

        void ProceduralPrefabSystemComponent::RegisterProceduralPrefab(const AZStd::string& prefabFilePath, TemplateId templateId)
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, prefabFilePath.c_str(),
                azrtti_typeid<AZ::Prefab::ProceduralPrefabAsset>(), false);

            if (assetId.IsValid())
            {
                AZStd::scoped_lock lock(m_lookupMutex);
                m_assetIdToTemplateLookup[assetId] = templateId;
            }
            else
            {
                AZ_Error("Prefab", false, "Failed to find AssetId for prefab %s", prefabFilePath.c_str());
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
