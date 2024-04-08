/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetPlatformComponentRemover.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabDocument.h>


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
            // No need to remove any components.
            return;
        }

        // Iterate over every entity, in every prefab
        prefabProcessorContext.ListPrefabs(
            [&prefabProcessorContext, &excludedComponents](PrefabDocument& prefab) -> void
            {
                prefab.GetInstance().GetAllEntitiesInHierarchy(
                    [&prefab, &prefabProcessorContext, &excludedComponents](AZStd::unique_ptr<AZ::Entity>& entity) -> bool
                    {
                        (void) prefab;

                        // Loop over an entity's components backwards and pop-off components that shouldn't exist.
                        AZStd::vector<AZ::Component*> components = entity->GetComponents();
                        const auto oldComponentCount = components.size();
                        for (int i = aznumeric_cast<int>(oldComponentCount) - 1; i >= 0; --i)
                        {
                            AZ::Component* component = components[i];
                            if (excludedComponents.contains(component->GetUnderlyingComponentType()))
                            {
                                entity->RemoveComponent(component);
                                delete component;
                            }
                        }

                        // Make sure we didn't remove any components that another component dependends on
                        if (oldComponentCount != entity->GetComponents().size())
                        {
                            if (entity->EvaluateDependencies() == AZ::Entity::DependencySortResult::MissingRequiredService)
                            {
                                AZ_Error( "AssetPlatformComponentRemover", false,
                                    "Processing prefab '%s' failed! Removing components on entity '%s' has broken component "
                                    "dependency. Make sure you also remove any dependent components. If dependent component is actually required, "
                                    "then keep the provider. Please update Amazon/Tools/Prefab/Processing/PlatformExcludedComponents settings registry (.setreg).",
                                    prefab.GetName().c_str(),
                                    entity->GetName().c_str()
                                );

                                prefabProcessorContext.ErrorEncountered();
                            }
                        }

                        // continue iterating over entities...
                        return true;
                    });
            });
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
