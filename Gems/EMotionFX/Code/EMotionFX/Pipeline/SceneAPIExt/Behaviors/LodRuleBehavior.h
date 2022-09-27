/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzSceneDef.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Group
        {
            class IActorGroup;
        }

        namespace Behavior
        {
            class LodRule;

            class LodRuleBehavior
                : public SceneCore::BehaviorComponent
                , public SceneEvents::ManifestMetaInfoBus::Handler
                , public SceneEvents::AssetImportRequestBus::Handler
                , public SceneEvents::GraphMetaInfoBus::Handler
            {
            public:
                AZ_COMPONENT(LodRuleBehavior, "{1F83C66C-44B1-4491-BCDE-F061A7E873AD}", SceneCore::BehaviorComponent);

                ~LodRuleBehavior() override = default;

                void Activate() override;
                void Deactivate() override;
                static void Reflect(AZ::ReflectContext* context);

                void InitializeObject(const SceneContainers::Scene& scene, SceneDataTypes::IManifestObject& target) override;
                static void BuildLODRuleForActor(const SceneContainers::Scene& scene, SceneDataTypes::IManifestObject& target);
                SceneEvents::ProcessingResult UpdateManifest(SceneContainers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;

                void GetVirtualTypeName(AZStd::string& name, AZ::Crc32 type) override;
                void GetPolicyName(AZStd::string& result) const override
                {
                    result = "LodRuleBehavior";
                }

            private:
                void UpdateLodRules(SceneContainers::Scene& scene) const;
            };
        }
    }
}
