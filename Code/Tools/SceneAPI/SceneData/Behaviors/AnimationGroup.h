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

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            class AnimationGroup
                : public SceneCore::BehaviorComponent
                , public Events::ManifestMetaInfoBus::Handler
                , public Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(AnimationGroup, "{CE7FEBE4-ACA3-41B8-9154-9B9E09A95A06}", SceneCore::BehaviorComponent);
                
                ~AnimationGroup() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(ReflectContext* context);

                // ManifestMetaInfo
                void GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene) override;
                void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
                
                // AssetImportRequest
                Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;
                
            private:
                Events::ProcessingResult BuildDefault(Containers::Scene& scene) const;
                Events::ProcessingResult UpdateAnimationGroups(Containers::Scene& scene) const;

                bool SceneHasAnimationGroup(const Containers::Scene& scene) const;

                static const int s_animationsPreferredTabOrder;
            };
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ