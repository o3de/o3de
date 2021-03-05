#pragma once

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

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPIExt/Rules/MeshRule.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            class MeshRuleBehavior
                : public AZ::SceneAPI::SceneCore::BehaviorComponent
                , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
                , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler                
            {
            public:
                AZ_COMPONENT(MeshRuleBehavior, "{8C5599B9-C46D-40F5-BC29-880415973654}", AZ::SceneAPI::SceneCore::BehaviorComponent);

                ~MeshRuleBehavior() override = default;

                // From BehaviorComponent
                void Activate() override;
                void Deactivate() override;
                static void Reflect(AZ::ReflectContext* context);

                void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;

                AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action, RequestingApplication requester) override;

            private:
                bool IsValidGroupType(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group) const;
                bool UpdateMeshRules(AZ::SceneAPI::Containers::Scene& scene);
                bool UpdateMeshRule(const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group, EMotionFX::Pipeline::Rule::MeshRule& meshRule);
            };
        } // Behavior
    } // Pipeline
} // EMotionFX
