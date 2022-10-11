/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzSceneDef.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            class SkeletonOptimizationRuleBehavior
                : public SceneCore::BehaviorComponent
                , public SceneEvents::ManifestMetaInfoBus::Handler
                , public SceneEvents::AssetImportRequestBus::Handler
            {
            public:
                AZ_COMPONENT(SkeletonOptimizationRuleBehavior, "{09D017C6-2F6E-4F64-895D-454205AD3E50}", SceneCore::BehaviorComponent);

                ~SkeletonOptimizationRuleBehavior() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(AZ::ReflectContext* context);

                void InitializeObject(const SceneContainers::Scene& scene, SceneDataTypes::IManifestObject& target) override;
                SceneEvents::ProcessingResult UpdateManifest(SceneContainers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;

                void GetPolicyName(AZStd::string& result) const override
                {
                    result = "SkeletonOptimizationRuleBehavior";
                }
            private:
                AZ::SceneAPI::Events::ProcessingResult UpdateSelection(AZ::SceneAPI::Containers::Scene& scene) const;
            };
        }
    }
}
