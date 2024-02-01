/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentExport.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/ranges/transform_view.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Spawnable/AssetPlatformComponentRemover.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

#pragma optimize("",off)
namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    AssetPlatformComponentRemover::~AssetPlatformComponentRemover()
    {
        for (auto* handler : m_editorOnlyEntityHandlerCandidates)
        {
            delete handler;
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

        prefabProcessorContext.ListPrefabs(
            [this, &serializeContext, &prefabProcessorContext](PrefabDocument& prefab)
            {
                auto result = RemoveComponentBasedOnAssetPlatform(prefab, serializeContext, prefabProcessorContext);
                if (!result)
                {
                    AZ_Error(
                        "Prefab", false, "Converting to runtime Prefab '%s' failed, Error: %s .", prefab.GetName().c_str(),
                        result.GetError().c_str());
                    return;
                } 
            });
    }

    void AssetPlatformComponentRemover::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<AssetPlatformComponentRemover, PrefabProcessor>()->Version(1);
        }
    }


    AssetPlatformComponentRemover::RemoveComponentBasedOnAssetPlatformResult AssetPlatformComponentRemover::RemoveComponentBasedOnAssetPlatform(
        PrefabDocument& prefab,
        AZ::SerializeContext* serializeContext,
        PrefabProcessorContext& prefabProcessorContext)
    {
        // replace entities of instance with exported ones.
        AZ::PlatformTagSet platformTags = prefabProcessorContext.GetPlatformTags();
        prefab.GetInstance().GetAllEntitiesInHierarchy(
            [&platformTags, &serializeContext](AZStd::unique_ptr<AZ::Entity>& entity)
            {
                platformTags;
                serializeContext;

                AZStd::vector<AZ::Component*> components = entity->GetComponents();
                for (auto component = components.rbegin(); component != components.rend(); ++component)
                {
                    component;
                }

                //for (AZ::Component* component : removedComponents)
                //{
                //    entity->RemoveComponent(component);
                //    delete component;
                //}



                //if (AZ::Component* meshComponent = entity->FindComponent(AZ::Uuid("{C7801FA8-3E82-4D40-B039-4854F1892FDE}")))
                //{
                //    entity->RemoveComponent(meshComponent);
                //    delete meshComponent;
                //}

                //if (AZ::Component* materialComponent = entity->FindComponent(AZ::Uuid("{E5A56D7F-C63E-4080-BF62-01326AC60982}")))
                //{
                //    entity->RemoveComponent(materialComponent);
                //    delete materialComponent;
                //}

                //if (AZ::Component* terrainMacroMaterialComponent = entity->FindComponent(AZ::Uuid("{F82379FB-E2AE-4F75-A6F4-1AE5F5DA42E8}")))
                //{
                //    entity->RemoveComponent(terrainMacroMaterialComponent);
                //    delete terrainMacroMaterialComponent;
                //}

                return true;
            }
        );

        return AZ::Success();
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
#pragma optimize("", on)
