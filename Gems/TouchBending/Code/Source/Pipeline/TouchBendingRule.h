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
#include <SceneAPI/SceneCore/DataTypes/Rules/ITouchBendingRule.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    class ReflectContext;
}

namespace TouchBending
{
    namespace Pipeline
    {
        /*!
          Read notes in ITouchBendingRule.h
         */
        class TouchBendingRule
            : public AZ::SceneAPI::DataTypes::ITouchBendingRule
        {
            friend class TouchBendingRuleBehavior;

        public:
            AZ_RTTI(TouchBendingRule, "{4B416987-2147-49DC-B725-C5DFDA51CB48}", AZ::SceneAPI::DataTypes::ITouchBendingRule);
            AZ_CLASS_ALLOCATOR_DECL

            TouchBendingRule();
            ~TouchBendingRule() override = default;

            const AZStd::string& GetRootBoneName() const override { return m_rootBoneName; }

            AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override { return m_proximityTriggerMeshes; }
            const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override { return m_proximityTriggerMeshes; }

            //Once support to parse FbxProperties (aka User Data Attributes) is added to the SceneAPI
            //the return values should come from member variables instead of constant true.
            //Refer To: LY-90723.
            bool ShouldOverrideStiffness() const override { return true; }
            float GetOverrideStiffness() const override { return m_stiffness; }

            bool ShouldOverrideDamping() const override { return true; }
            float GetOverrideDamping() const override { return m_damping; }

            bool ShouldOverrideThickness() const override { return true; }
            float GetOverrideThickness() const override { return m_thickness; }

            static void Reflect(AZ::ReflectContext* context);

        protected:
            AZStd::string m_rootBoneName;

            /*!
              This is usually a single mesh. The idea is that a brand new NoCollide CryPhysics Material
              will be created to label the mesh as not drawable and to be used as the proximity trigger to start
              touchBending simulation of the vegetation geometry. 
              
              In the future, the expectation is that Extra Atrributes, aka UDP, aka Custom Attributes defined inside
              nodes of FBX Files will be exposed to the SceneAPI. In the meantime, the user can name the proximity
              trigger mesh as "*_touchbend" (or as the user customizes the matching pattern)
              if they expect the mesh to be detected automatically by the FBX Pipeline.
              //Refer To: LY-90723.
             */
            AZ::SceneAPI::SceneData::SceneNodeSelectionList m_proximityTriggerMeshes;

            float m_stiffness;
            float m_damping;
            float m_thickness;
        };
    } // Pipeline
} // TouchBending