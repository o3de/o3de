/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Character.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsMaterialConversionUtils.h>

#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

#include <Source/Integration/Assets/ActorAsset.h>

#include <SceneAPIExt/Groups/ActorGroup.h>
#include <SceneAPIExt/Rules/ActorPhysicsSetupRule.h>
#include <SceneAPIExt/Utilities/LegacyPhysicsMaterialFbxManifestConversion.h>

namespace EMotionFX::Pipeline::Utilities
{
    // Converts legacy material selection inside character collider configuration
    // into new material slots.
    bool FixCharacterColliderConfiguration(
        Physics::CharacterColliderConfiguration& characterColliderConfiguration,
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        bool characterColliderConfigurationModified = false;

        for (auto& characterColliderNodeConfiguration : characterColliderConfiguration.m_nodes)
        {
            for (auto& shapeColliderPair : characterColliderNodeConfiguration.m_shapes)
            {
                AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfiguration = shapeColliderPair.first;
                if (!colliderConfiguration)
                {
                    continue;
                }

                Physics::MaterialSlots materialSlots =
                    Physics::Utils::ConvertLegacyMaterialSelectionToMaterialSlots(colliderConfiguration->m_legacyMaterialSelection, legacyMaterialIdToNewAssetIdMap);

                if (Physics::Utils::IsDefaultMaterialSlots(materialSlots))
                {
                    continue;
                }

                AZ_TracePrintf("EMFXMaterialConversion", "Legacy material selection will be replaced by physics material slots.\n");
                if (!colliderConfiguration->m_legacyMaterialSelection.m_materialIdsAssignedToSlots.empty())
                {
                    AZ_Assert(
                        colliderConfiguration->m_legacyMaterialSelection.m_materialIdsAssignedToSlots.size() == materialSlots.GetSlotsCount(),
                        "Number of elements in legacy material selection (%zu) and material slots (%zu) do not match.",
                        colliderConfiguration->m_legacyMaterialSelection.m_materialIdsAssignedToSlots.size(), materialSlots.GetSlotsCount());

                    for (size_t i = 0; i < materialSlots.GetSlotsCount(); ++i)
                    {
                        AZ_TracePrintf(
                            "EMFXMaterialConversion", "  Slot %zu '%.*s') Legacy material id '%s' -> material asset '%s'.\n", i + 1,
                            AZ_STRING_ARG(materialSlots.GetSlotName(i)),
                            colliderConfiguration->m_legacyMaterialSelection.m_materialIdsAssignedToSlots[i].m_id.ToString<AZStd::string>().c_str(),
                            materialSlots.GetMaterialAsset(i).GetHint().c_str());
                    }
                }

                colliderConfiguration->m_materialSlots = materialSlots;
                colliderConfiguration->m_legacyMaterialSelection = {};

                characterColliderConfigurationModified = true;
            }
        }

        return characterColliderConfigurationModified;
    }

    // Converts all legacy material selections found inside physics setup rule
    // into new material slots.
    bool FixActorPhysicsSetupRule(
        EMotionFX::Pipeline::Rule::ActorPhysicsSetupRule& actorPhysicsSetupRule,
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        AZStd::shared_ptr<EMotionFX::PhysicsSetup> newPhysicsSetup;
        {
            const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actorPhysicsSetupRule.GetData();
            if (!physicsSetup)
            {
                AZ_Warning(
                    "EMFXMaterialConversion", false, "ActorPhysicsSetupRule with invalid data.");
                return false;
            }
            newPhysicsSetup = AZStd::make_shared<EMotionFX::PhysicsSetup>(*physicsSetup); // make a copy of the data to amend
        }

        bool physicsSetupModified = false;

        Physics::AnimationConfiguration& animationConfiguration = newPhysicsSetup->GetConfig();

        if (FixCharacterColliderConfiguration(animationConfiguration.m_hitDetectionConfig, legacyMaterialIdToNewAssetIdMap))
        {
            physicsSetupModified = true;
        }

        if (FixCharacterColliderConfiguration(animationConfiguration.m_ragdollConfig.m_colliders, legacyMaterialIdToNewAssetIdMap))
        {
            physicsSetupModified = true;
        }

        if (FixCharacterColliderConfiguration(animationConfiguration.m_clothConfig, legacyMaterialIdToNewAssetIdMap))
        {
            physicsSetupModified = true;
        }

        if (FixCharacterColliderConfiguration(animationConfiguration.m_simulatedObjectColliderConfig, legacyMaterialIdToNewAssetIdMap))
        {
            physicsSetupModified = true;
        }

        if (physicsSetupModified)
        {
            actorPhysicsSetupRule.SetData(newPhysicsSetup);
        }

        return physicsSetupModified;
    }

    // Converts all legacy material selections found inside an FBX
    // manifest (Actor Group) into new material slots.
    void FixFbxManifestPhysicsMaterials(
        const AZStd::string& fbxManifestFullPath,
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        AZ::SceneAPI::Containers::SceneManifest sceneManifest;
        if (!sceneManifest.LoadFromFile(fbxManifestFullPath))
        {
            AZ_Warning("EMFXMaterialConversion", false, "Unable to load FBX manifest '%s'.", fbxManifestFullPath.c_str());
            return;
        }

        AZ::SceneAPI::Containers::SceneManifest::ValueStorageData valueStorage = sceneManifest.GetValueStorage();
        auto actorGroupFilterView = AZ::SceneAPI::Containers::MakeDerivedFilterView<EMotionFX::Pipeline::Group::ActorGroup>(valueStorage);

        bool fbxManifestModified = false;
        for (auto& actorGroup : actorGroupFilterView)
        {
            auto& rules = actorGroup.GetRuleContainer();
            for (size_t i = 0; i < rules.GetRuleCount(); ++i)
            {
                AZStd::shared_ptr<AZ::SceneAPI::DataTypes::IRule> rule = rules.GetRule(i);
                if (rule && rule->RTTI_IsTypeOf(EMotionFX::Pipeline::Rule::ActorPhysicsSetupRule::TYPEINFO_Uuid()))
                {
                    if (FixActorPhysicsSetupRule(
                        *AZStd::static_pointer_cast<EMotionFX::Pipeline::Rule::ActorPhysicsSetupRule>(rule),
                        legacyMaterialIdToNewAssetIdMap))
                    {
                        fbxManifestModified = true;
                    }
                }
            }
        }

        if (fbxManifestModified)
        {
            AZ_TracePrintf("EMFXMaterialConversion", "Saving fbx manifest '%s'.\n", fbxManifestFullPath.c_str());

            // Request source control to edit prefab file
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, fbxManifestFullPath.c_str(), true,
                [fbxManifestFullPath, sceneManifest]([[maybe_unused]] bool success, const AzToolsFramework::SourceControlFileInfo& info) mutable
                {
                    // This is called from the main thread on the next frame from TickBus,
                    // that is why 'fbxManifestFullPath' and 'fbxManifest' are captured as a copy.
                    // The lambda also has to be mutable because `SaveToFile` is not const.
                    if (!info.IsReadOnly())
                    {
                        if (!sceneManifest.SaveToFile(fbxManifestFullPath))
                        {
                            AZ_Warning("EMFXMaterialConversion", false, "Unable to save prefab '%s'", fbxManifestFullPath.c_str());
                        }
                    }
                    else
                    {
                        AZ_Warning(
                            "EMFXMaterialConversion", false, "Unable to check out asset '%s' in source control.",
                            fbxManifestFullPath.c_str());
                    }
                });

            AZ_TracePrintf("EMFXMaterialConversion", "\n");
        }
    }

    void FixFbxManifestsWithPhysicsLegacyMaterials(const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        AZ_TracePrintf("EMFXMaterialConversion", "Searching for FBX manifests with actor assets...\n");
        AZ_TracePrintf("EMFXMaterialConversion", "\n");

        const AZStd::set<AZStd::string> fbxManifests = Physics::Utils::CollectFbxManifestsFromAssetType(EMotionFX::Integration::ActorAsset::RTTI_Type());
        if (fbxManifests.empty())
        {
            AZ_TracePrintf("EMFXMaterialConversion", "No FBX manifests found.\n");
            AZ_TracePrintf("EMFXMaterialConversion", "\n");
            return;
        }
        AZ_TracePrintf("EMFXMaterialConversion", "Found %zu FBX manifests to check.\n", fbxManifests.size());
        AZ_TracePrintf("EMFXMaterialConversion", "\n");

        for (const auto& fbxManifest : fbxManifests)
        {
            FixFbxManifestPhysicsMaterials(fbxManifest, legacyMaterialIdToNewAssetIdMap);
        }

        AZ_TracePrintf("EMFXMaterialConversion", "FBX manifests conversion finished.\n");
        AZ_TracePrintf("EMFXMaterialConversion", "\n");
    }
} // namespace EMotionFX::Pipeline::Utilities
