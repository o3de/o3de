/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Console/IConsole.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <EditorColliderComponent.h>
#include <EditorShapeColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <EditorStaticRigidBodyComponent.h>

#include <Editor/Source/Components/Conversion/PrefabConversionUtils.h>

namespace PhysX
{
    void UpdatePrefabsWithColliderComponents(const AZ::ConsoleCommandContainer& commandArgs);

    AZ_CONSOLEFREEFUNC(
        "ed_physxUpdatePrefabsWithColliderComponents",
        UpdatePrefabsWithColliderComponents,
        AZ::ConsoleFunctorFlags::Null,
        "Finds entities with collider components and no rigid bodies and updates them to the new pattern which requires a static rigid body component.");

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
                "Unable to load entity '%s' from prefab '%s'.",
                entity.GetName().c_str(),
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
                    return typeId == azrtti_typeid<EditorColliderComponent>() || typeId == azrtti_typeid<EditorShapeColliderComponent>();
                });

            if (rigidBodyMissing && colliderPresent)
            {
                if (AddStaticRigidBodyToPrefabEntity(prefabInfo, *entity))
                {
                    prefabModified = true;
                }
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
        bool prefabSystemEnabled = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            prefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);
        if (!prefabSystemEnabled)
        {
            AZ_TracePrintf("PhysXColliderConversion", "Prefabs system is not enabled. Prefabs won't be converted.\n");
            AZ_TracePrintf("PhysXColliderConversion", "\n");
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
