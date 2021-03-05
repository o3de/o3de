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
                
            private:
                AZ::SceneAPI::Events::ProcessingResult BuildDefault(AZ::SceneAPI::Containers::Scene& scene) const;
                AZ::SceneAPI::Events::ProcessingResult UpdateMotionGroupBehaviors(AZ::SceneAPI::Containers::Scene& scene) const;

                bool SceneHasMotionGroup(const AZ::SceneAPI::Containers::Scene& scene) const;

                static const int s_preferredTabOrder;
            };
        } // Behavior
    } // Pipeline
} // EMotionFX