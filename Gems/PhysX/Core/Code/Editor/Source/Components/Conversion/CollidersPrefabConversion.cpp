/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Console/IConsole.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <EditorColliderComponent.h>
#include <EditorMeshColliderComponent.h>
#include <EditorShapeColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <EditorStaticRigidBodyComponent.h>

#include <Editor/Source/Components/Conversion/PrefabConversionUtils.h>

namespace PhysX
{
    void UpdatePrefabsWithColliderComponents(const AZ::ConsoleCommandContainer& commandArgs);

    // O3DE_DEPRECATION_NOTICE(GHI-14718)
    AZ_CONSOLEFREEFUNC(
        "ed_physxUpdatePrefabsWithColliderComponents",
        UpdatePrefabsWithColliderComponents,
        AZ::ConsoleFunctorFlags::Null,
        "Finds entities with collider components and no rigid bodies and updates them to the new pattern which requires a static rigid body component. "
        "Finds entities with collider components using physx asset and replace it with a mesh collider component.");

    bool AddStaticRigidBodyToPrefabEntity(
        Utils::PrefabInfo& prefabInfo,
        AzToolsFramework::Prefab::PrefabDomValue& entityPrefab)
    {
        AZ::Entity entity;
        Utils::PrefabEntityIdMapper prefabEntityIdMapper;

        if (!Utils::LoadPrefabEntity(prefabEntityIdMapper, entityPrefab, entity))
        {
            AZ_Warning(
                "PhysXColliderConversion",
                false,
                "Unable to load entity from prefab '%s'.",
                prefabInfo.m_prefabFullPath.c_str());
            return false;
        }

        if (!entity.CreateComponent<EditorStaticRigidBodyComponent>())
        {
            AZ_Warning(
                "PhysXColliderConversion",
                false,
                "Failed to create static rigid body component for entity '%s' in prefab '%s'.",
                entity.GetName().c_str(),
                prefabInfo.m_prefabFullPath.c_str());
            return false;
        }

        if (!Utils::StorePrefabEntity(prefabEntityIdMapper, prefabInfo.m_template->GetPrefabDom(), entityPrefab, entity))
        {
            AZ_Warning(
                "PhysXColliderConversion",
                false,
                "Unable to store entity '%s' into prefab '%s'.",
                entity.GetName().c_str(),
                prefabInfo.m_prefabFullPath.c_str());
            return false;
        }

        return true;
    }

    bool ConvertCollidersUsingAssetsToMeshCollidersInPrefabEntity(
        Utils::PrefabInfo& prefabInfo,
        AzToolsFramework::Prefab::PrefabDomValue& entityPrefab)
    {
        AZ::Entity entity;
        Utils::PrefabEntityIdMapper prefabEntityIdMapper;

        if (!Utils::LoadPrefabEntity(prefabEntityIdMapper, entityPrefab, entity))
        {
            AZ_Warning(
                "PhysXColliderConversion",
                false,
                "Unable to load entity from prefab '%s'.",
                prefabInfo.m_prefabFullPath.c_str());
            return false;
        }

        bool entityModified = false;

        for (auto* editorColliderComponent : entity.FindComponents<EditorColliderComponent>())
        {
            const EditorProxyShapeConfig& proxyShapeConfig = editorColliderComponent->GetShapeConfiguration();

            if (proxyShapeConfig.m_shapeType != Physics::ShapeType::PhysicsAsset)
            {
                continue;
            }

            EditorProxyAssetShapeConfig newProxyAssetShapeConfig;
            newProxyAssetShapeConfig.m_physicsAsset.m_configuration = proxyShapeConfig.m_legacyPhysicsAsset.m_configuration;
            newProxyAssetShapeConfig.m_physicsAsset.m_pxAsset = proxyShapeConfig.m_legacyPhysicsAsset.m_pxAsset;
            newProxyAssetShapeConfig.m_hasNonUniformScale = proxyShapeConfig.m_hasNonUniformScale;
            newProxyAssetShapeConfig.m_subdivisionLevel = proxyShapeConfig.m_subdivisionLevel;

            auto* editorMeshColliderComponent = entity.CreateComponent<EditorMeshColliderComponent>(
                editorColliderComponent->GetColliderConfiguration(), newProxyAssetShapeConfig, editorColliderComponent->IsDebugDrawDisplayFlagEnabled());
            if (!editorMeshColliderComponent)
            {
                AZ_Warning(
                    "PhysXColliderConversion",
                    false,
                    "Failed to create static rigid body component for entity '%s' in prefab '%s'.",
                    entity.GetName().c_str(),
                    prefabInfo.m_prefabFullPath.c_str());
                return false;
            }

            if (!entity.RemoveComponent(editorColliderComponent))
            {
                AZ_Warning(
                    "PhysXColliderConversion",
                    false,
                    "Failed to remove EditorColliderComponent in entity '%s' in prefab '%s'.",
                    entity.GetName().c_str(),
                    prefabInfo.m_prefabFullPath.c_str());
                return false;
            }
            // Keep the same component id for the mesh collider component. It's needed
            // in case there are other prefabs with patches referencing the old component.
            editorMeshColliderComponent->SetId(editorColliderComponent->GetId());

            // Once the component is removed from the entity we are responsible for its destruction.
            delete editorColliderComponent;

            entityModified = true;
        }

        if (!Utils::StorePrefabEntity(prefabEntityIdMapper, prefabInfo.m_template->GetPrefabDom(), entityPrefab, entity))
        {
            AZ_Warning(
                "PhysXColliderConversion",
                false,
                "Unable to store entity '%s' into prefab '%s'.",
                entity.GetName().c_str(),
                prefabInfo.m_prefabFullPath.c_str());
            return false;
        }

        return entityModified;
    }

    void UpdatePrefabPhysXColliders(Utils::PrefabInfo& prefabInfo)
    {
        bool prefabModified = false;
        for (auto* entity : Utils::GetPrefabEntities(prefabInfo.m_template->GetPrefabDom()))
        {
            const auto entityComponents = Utils::GetEntityComponents(*entity);

            const bool rigidBodyMissing = AZStd::none_of(
                entityComponents.begin(),
                entityComponents.end(),
                [](const auto* component)
                {
                    const auto typeId = Utils::GetComponentTypeId(*component);
                    return typeId == azrtti_typeid<EditorRigidBodyComponent>() || typeId == azrtti_typeid<EditorStaticRigidBodyComponent>();
                });

            const bool colliderPresent = AZStd::any_of(
                entityComponents.begin(),
                entityComponents.end(),
                [](const auto* component)
                {
                    const auto typeId = Utils::GetComponentTypeId(*component);
                    return typeId == azrtti_typeid<EditorColliderComponent>()
                        || typeId == azrtti_typeid<EditorMeshColliderComponent>()
                        || typeId == azrtti_typeid<EditorShapeColliderComponent>();
                });

            // Adds a static rigid body to entities with a collider and no rigid body present.
            if (rigidBodyMissing && colliderPresent)
            {
                if (AddStaticRigidBodyToPrefabEntity(prefabInfo, *entity))
                {
                    prefabModified = true;
                }
            }

            // Converts all EditorColliderComponent that use physics assets to EditorMeshColliderComponent.
            if (ConvertCollidersUsingAssetsToMeshCollidersInPrefabEntity(prefabInfo, *entity))
            {
                prefabModified = true;
            }
        }

        if (prefabModified)
        {
            AZ_TracePrintf("PhysXColliderConversion", "Saving modified prefab '%s'.\n", prefabInfo.m_prefabFullPath.c_str());

            Utils::SavePrefab(prefabInfo);

            AZ_TracePrintf("PhysXColliderConversion", "\n");
        }
    }

    void UpdatePrefabsWithColliderComponents([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs)
    {
        bool isLevelOpen = false;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
            isLevelOpen, &AzToolsFramework::EditorRequests::Bus::Events::IsLevelDocumentOpen);
        if (isLevelOpen)
        {
            AZ_Warning(
                "PhysXColliderConversion", false,
                "There is a level currently opened in the editor. To run this command please restart the editor and run it before opening "
                "any level.\n");
            return;
        }

        AZ_TracePrintf("PhysXColliderConversion", "Searching for prefabs to convert...\n");
        AZ_TracePrintf("PhysXColliderConversion", "\n");

        AZStd::vector<Utils::PrefabInfo> prefabs = Utils::CollectPrefabs();
        if (prefabs.empty())
        {
            AZ_TracePrintf("PhysXColliderConversion", "No prefabs found.\n");
            AZ_TracePrintf("PhysXColliderConversion", "\n");
            return;
        }
        AZ_TracePrintf("PhysXColliderConversion", "Found %zu prefabs to check.\n", prefabs.size());
        AZ_TracePrintf("PhysXColliderConversion", "\n");

        for (auto& prefab : prefabs)
        {
            UpdatePrefabPhysXColliders(prefab);
        }

        AZ_TracePrintf("PhysXColliderConversion", "Prefab conversion finished.\n");
        AZ_TracePrintf("PhysXColliderConversion", "\n");
    }
} // namespace PhysX
