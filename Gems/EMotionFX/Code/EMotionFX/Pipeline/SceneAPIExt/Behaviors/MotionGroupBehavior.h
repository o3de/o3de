/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <AzCore/Memory/Memory.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            class MotionGroupBehavior
                : public AZ::SceneAPI::SceneCore::BehaviorComponent
                , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
                , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(MotionGroupBehavior, "{643EAD72-FD50-4771-8D88-78E617D92C6D}", AZ::SceneAPI::SceneCore::BehaviorComponent);

                ~MotionGroupBehavior() override = default;

                // From BehaviorComponent
                void Activate() override;
                void Deactivate() override;
                static void Reflect(AZ::ReflectContext* context);

                // ManifestMetaInfo
                void GetCategoryAssignments(CategoryRegistrationList& categories, const AZ::SceneAPI::Containers::Scene& scene) override;
                void GetAvailableModifiers(ModifiersList& modifiers, const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IManifestObject& target) override;
                void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;
                
                // AssetImportRequest
                AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;

                void GetPolicyName(AZStd::string& result) const override
                {
                    result = "MotionGroupBehavior";
                }

            private:
                AZ::SceneAPI::Events::ProcessingResult BuildDefault(AZ::SceneAPI::Containers::Scene& scene) const;
                AZ::SceneAPI::Events::ProcessingResult UpdateMotionGroupBehaviors(AZ::SceneAPI::Containers::Scene& scene) const;

                bool SceneHasMotionGroup(const AZ::SceneAPI::Containers::Scene& scene) const;

                static constexpr int s_motionGroupPreferredTabOrder{ 2 };
            };
        } // Behavior
    } // Pipeline
} // EMotionFX
