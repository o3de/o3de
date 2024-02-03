/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Spawnable/AssetPlatformComponentRemover.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabDocument.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessorContext.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    void AssetPlatformComponentRemover::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetPlatformComponentRemover, PrefabProcessor>()
                ->Version(1)
                ->Field("PlatformExcludedComponents", &AssetPlatformComponentRemover::m_platformExcludedComponents)
                ;
        }
    }

    void AssetPlatformComponentRemover::Process(PrefabProcessorContext& prefabProcessorContext)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Assert(serializeContext, "Failed to retrieve serialize context.");
            return;
        }

        AZ::PlatformTagSet platformTags = prefabProcessorContext.GetPlatformTags();
        AZStd::set<AZ::Uuid> excludedComponents;
        for (const auto& platforms : m_platformExcludedComponents)
        {
            if (platformTags.contains(AZ::Crc32(platforms.first)))
            {
                excludedComponents.insert_range(platforms.second);
            }
        }

        if (excludedComponents.empty())
        {
            return;
        }
        
        prefabProcessorContext.ListPrefabs(
            [this, &excludedComponents](PrefabDocument& prefab)
            {
                RemoveComponentsBasedOnAssetPlatform(prefab, excludedComponents);
            });
    }

    void AssetPlatformComponentRemover::RemoveComponentsBasedOnAssetPlatform(PrefabDocument& prefab, const AZStd::set<AZ::Uuid>& exludedComponents)
    {
        if (exludedComponents.empty())
        {
            return;
        }

        prefab.GetInstance().GetAllEntitiesInHierarchy(
            [&exludedComponents](AZStd::unique_ptr<AZ::Entity>& entity)
            {
                AZStd::vector<AZ::Component*> components = entity->GetComponents();
                for (auto component = components.rbegin(); component != components.rend(); ++component)
                {
                    if (exludedComponents.contains((*component)->GetUnderlyingComponentType()))
                    {
                        entity->RemoveComponent(*component);
                        delete *component;
                    }
                }

                return true;
            }
        );
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
