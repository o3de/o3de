/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Builders/MeshletPackRuleBehavior.h>
#include <Builders/MeshletPackRule.h>

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_set.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>

namespace AZ::Meshlets::Builders
{
    void MeshletPackRuleBehavior::Activate()
    {
        AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
    }

    void MeshletPackRuleBehavior::Deactivate()
    {
        AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
    }

    void MeshletPackRuleBehavior::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MeshletPackRuleBehavior,
                                    AZ::SceneAPI::SceneCore::BehaviorComponent>()
                ->Version(1);
        }
    }

    void MeshletPackRuleBehavior::GetAvailableModifiers(
        AZ::SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers,
        [[maybe_unused]] const AZ::SceneAPI::Containers::Scene& scene,
        const AZ::SceneAPI::DataTypes::IManifestObject& target)
    {
        // Only mesh groups (static models) can carry a meshlet pack rule.
        if (!target.RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IMeshGroup::TYPEINFO_Uuid()))
        {
            return;
        }

        const auto* group = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshGroup*>(&target);
        if (!group)
        {
            return;
        }

        // A group accepts at most one MeshletPackRule. If it already has one,
        // don't offer it again (matches the existing-rule guard used by the
        // stock ManifestMetaInfoHandler for the built-in rules).
        const AZ::SceneAPI::Containers::RuleContainer& rules = group->GetRuleContainerConst();
        const size_t ruleCount = rules.GetRuleCount();
        for (size_t i = 0; i < ruleCount; ++i)
        {
            if (rules.GetRule(i)->RTTI_GetType() == azrtti_typeid<MeshletPackRule>())
            {
                return;
            }
        }

        modifiers.push_back(azrtti_typeid<MeshletPackRule>());
    }

    void MeshletPackRuleBehavior::InitializeObject(
        [[maybe_unused]] const AZ::SceneAPI::Containers::Scene& scene,
        AZ::SceneAPI::DataTypes::IManifestObject& target)
    {
        // Called whenever a manifest object is created in the editor. When that
        // object is our rule (i.e. the user just added it from the dropdown),
        // stamp the documented sane defaults onto it.
        if (auto* rule = azrtti_cast<MeshletPackRule*>(&target))
        {
            rule->SetDefaults();
        }
    }

} // namespace AZ::Meshlets::Builders
