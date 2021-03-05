/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

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
                
            private:
                AZ::SceneAPI::Events::ProcessingResult BuildDefault(AZ::SceneAPI::Containers::Scene& scene) const;
                AZ::SceneAPI::Events::ProcessingResult UpdateActorGroups(AZ::SceneAPI::Containers::Scene& scene) const;

                bool SceneHasActorGroup(const AZ::SceneAPI::Containers::Scene& scene) const;

                static const int s_animationsPreferredTabOrder;
            };
        } // Behavior
    } // Pipeline
} // EMotionFX