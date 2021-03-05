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

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            /*!
              The author of the touch bendable asset will be able to define stiffness, damping and thickness
              as attributes per bone. Adding those kind of attributes to a node is supported
              by DCC tools and the FBX API (FbxProperty), but SceneAPI doesn't support parsing those attributes into the
              Scene Nodes or IGraphObjects. In the meantime, and while parsing Extra Atrributes is not supported by SceneAPI, the
              content author should manually set the values in this Modifier. The values will be applied to all bones.

              The ShouldOverride*() methods are put in place so in the future the content author can override
              all the attributes at once without re-exporting assets from the DCC tools.
              This is more of a convenience.

              If the original asset doesn't have any of these attributes in the bones, then the values
              of this rule will be applied to the root bone. Children bones will copy the parents attributes
              for all unspecified attributes.
             */
            class ITouchBendingRule
                : public IRule
            {
            public:
                AZ_RTTI(ITouchBendingRule, "{2FE2B499-DB71-4D69-8944-6DE2396D6E78}", IRule);

                ~ITouchBendingRule() override = default;

                virtual const AZStd::string& GetRootBoneName() const = 0;

                /*!
                  The returned list contains only one mesh in 99.99% of the cases.
                  The mesh is supposed to be the proximity trigger mesh.
                  Most of the time the content author would want the proximity trigger mesh
                  to be as simple as possible for performance reasons. So, something like a simple
                  cube that covers the main render mesh is the ideal thing to do for collision detection.
                  The selected proximity mesh is stored in a list, because there can be extreme cases
                  where several meshes can define the proximity trigger volume. They will all be combined
                  into a single submesh into the exported CGF file, at the expense of performance when the
                  engine calculates if a vegetation mesh is being touched or not.
                 */
                virtual ISceneNodeSelectionList& GetSceneNodeSelectionList() = 0;
                virtual const ISceneNodeSelectionList& GetSceneNodeSelectionList() const = 0;

                ///If true, The stifness parameter for all the bones
                ///in the tree will be set to GetOverrideStiffness(), replacing
                ///the value set by the Author of the asset.
                virtual bool ShouldOverrideStiffness() const = 0;
                ///A value from 0.0f to 1.0f.
                ///0.0 means no stiffness, the tree will look like a sad willow.
                ///Segments (bones) of the tree would never return to its original pose
                ///after being pushed by a Collider.
                virtual float GetOverrideStiffness() const = 0;

                ///If true, The Damping parameter for all the bones
                ///in the tree will be set to GetOverrideDamping(), replacing
                ///the value set by the Author of the asset.
                virtual bool ShouldOverrideDamping() const = 0;
                ///A value from 0.0 to 1.0. 0.0 means no damping, lots of back and forth movement around its original pose.
                ///1.0 means maximum damping, the segment will quickly converge back to its original pose.
                virtual float GetOverrideDamping() const = 0;

                ///If true, The thickness parameter for all the bones
                ///in the tree will be set to GetOverrideThickness(), replacing
                ///the value set by the Author of the asset.
                virtual bool ShouldOverrideThickness() const = 0;
                ///If you imagine the Segment (or Bone) to be a cylinder, this is its radius in meters.
                virtual float GetOverrideThickness() const = 0;

            }; //class ITouchBendingRule
        } //namespace DataTypes
    }  // namespace SceneAPI
}  // namespace AZ