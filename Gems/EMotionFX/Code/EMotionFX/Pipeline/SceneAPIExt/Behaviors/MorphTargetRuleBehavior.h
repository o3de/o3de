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
                void GetPolicyName(AZStd::string& result) const override
                {
                    result = "MorphTargetRuleBehavior";
                }
            private:
                void UpdateMorphTargetRules(const AZ::SceneAPI::Containers::Scene& scene) const;
            };
        } // namespace Behavior
    } // namespace Pipeline
} // namespace EMotionFX
