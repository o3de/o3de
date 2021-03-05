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
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace TouchBending
{
    namespace Pipeline
    {
        /*!
          The following BehaviorComponent \dev\Code\Tools\SceneAPI\SceneData\Behaviors\MeshGroup.cpp owns
          the creation of the MeshGroup (The data, NOT the behavior).
          A plain CGF file is usually a static geometry file with no Bones, no skinning info, just a simple mesh.
          So, in principle, the MeshGroup Behavior would end up skipping the creation of the MeshGroup Data if it finds the file has Bones.
          TouchBending changes that, and it is possible to have bones and skinning information inside a CGF.
          So, if we find a mesh whose name matches "*_touchbend" this is the explicit signal we need to declare
          the asset as touch bendable.
          If the user names the root bone so it also matches "*_touchbend",
          coincidentally the MeshGroup Behavior will figure out it is a special bone so it also ends up creating the mesh group
          automatically.
        */
        class TouchBendingRuleBehavior
            : public AZ::SceneAPI::SceneCore::BehaviorComponent
            , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
            , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
        {
        public:
            AZ_COMPONENT(TouchBendingRuleBehavior, "{C32D84E5-E92E-46D3-887E-43A75AB0B435}", AZ::SceneAPI::SceneCore::BehaviorComponent);

            ~TouchBendingRuleBehavior() override = default;

            void Activate() override;
            void Deactivate() override;
            static void Reflect(AZ::ReflectContext* context);

            ////////  For ManifestMetaInfoBus START
            void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;

            void GetAvailableModifiers(AZ::SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers,
                const AZ::SceneAPI::Containers::Scene& scene,
                const AZ::SceneAPI::DataTypes::IManifestObject& target) override;
            ////////  For ManifestMetaInfoBus END

            ////////  For AssetImportRequestBus START
            AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
                RequestingApplication requester) override;
            ////////  For AssetImportRequestBus END

        private:
            AZ::SceneAPI::Events::ProcessingResult UpdateTouchBendingRule(AZ::SceneAPI::Containers::Scene& scene) const;

            /*!
              For practical and real life scenarios one mesh should be returned.
              \p selection can be nullptr, and in this case the method returns as soon as its find a mesh
              named for TouchBending proximity trigger.
             */
            size_t SelectProximityTriggerMeshes(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::ISceneNodeSelectionList* selection) const;

            /*!
              \param scene
              \param selectedBoneName Name of preselected root bone. Can be null.
              \returns -  if \p rootBoneName is null, Looks for the first Bone node with virtual type "TouchBend". If found returns
                          the name of such node.
                       -  if \p rootBoneName is NOT null, makes sure a bone with such name exists and returns the name of such node,
                          if a bone with such name is not found, then it Looks for the first Bone node with virtual type "TouchBend".
                          If found returns the name of such node.
                       -  An empty string if none of the above scenarios succeeds.
            */
            AZStd::string FindRootBoneName(const AZ::SceneAPI::Containers::Scene& scene, const char *selectedBoneName) const;
        };
    } // Pipeline
} // TouchBending
