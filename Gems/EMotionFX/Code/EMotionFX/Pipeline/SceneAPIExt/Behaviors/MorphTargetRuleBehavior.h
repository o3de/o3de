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
        namespace DataTypes
        {
            class IGroup;
        }
    }
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            class MorphTargetRuleBehavior 
                : public AZ::SceneAPI::SceneCore::BehaviorComponent
                , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
                , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(MorphTargetRuleBehavior, "{C1489B30-783D-40FC-93C6-F8D7F28DA6EA}", AZ::SceneAPI::SceneCore::BehaviorComponent);

                ~MorphTargetRuleBehavior() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(AZ::ReflectContext* context);

                void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;
                AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;

            private:
                void UpdateMorphTargetRules(const AZ::SceneAPI::Containers::Scene& scene) const;
            };
        } // namespace Behavior
    } // namespace Pipeline
} // namespace EMotionFX
