/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            class ActorGroupBehavior
                : public AZ::SceneAPI::SceneCore::BehaviorComponent
                , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
                , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
                , public AZ::SceneAPI::Events::GraphMetaInfoBus::Handler
            {
            public:
                AZ_COMPONENT(ActorGroupBehavior, "{D470A655-31ED-491E-A3FD-4BA3C75C0EDE}", AZ::SceneAPI::SceneCore::BehaviorComponent);

                ~ActorGroupBehavior() override = default;

                // From BehaviorComponent
                void Activate() override;
                void Deactivate() override;
                static void Reflect(AZ::ReflectContext* context);

                // ManifestMetaInfo
                void GetCategoryAssignments(CategoryRegistrationList& categories, const AZ::SceneAPI::Containers::Scene& scene) override;
                void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;
                
                // AssetImportRequest
                AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;

                void GetAvailableModifiers(ModifiersList& modifiers, const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IManifestObject& target) override;
                void GetPolicyName(AZStd::string& result) const override
                {
                    result = "ActorGroupBehavior";
                }

                // GraphMetaInfo
                void GetAppliedPolicyNames(AZStd::set<AZStd::string>& appliedPolicies, const AZ::SceneAPI::Containers::Scene& scene) const override;

            private:
                AZ::SceneAPI::Events::ProcessingResult BuildDefault(AZ::SceneAPI::Containers::Scene& scene) const;
                AZ::SceneAPI::Events::ProcessingResult UpdateActorGroups(AZ::SceneAPI::Containers::Scene& scene) const;

                bool SceneHasActorGroup(const AZ::SceneAPI::Containers::Scene& scene) const;

                static constexpr int s_actorsPreferredTabOrder{ 3 };
            };
        } // Behavior
    } // Pipeline
} // EMotionFX
