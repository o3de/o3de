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

            private:
                void UpdateLodRules(SceneContainers::Scene& scene) const;
            };
        }
    }
}
