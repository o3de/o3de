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

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Group
        {
            class IActorGroup
                : public AZ::SceneAPI::DataTypes::ISceneNodeGroup
            {
            public:
                AZ_RTTI(IActorGroup, "{C86945A8-AEE8-4CFC-8FBF-A20E9BC71348}", AZ::SceneAPI::DataTypes::ISceneNodeGroup);

                ~IActorGroup() override = default;

                virtual const AZStd::string& GetSelectedRootBone() const = 0;

                virtual void SetSelectedRootBone(const AZStd::string& selectedRootBone) = 0;

                // This list should contain the base meshe nodes (LOD0 meshes)
                virtual AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetBaseNodeSelectionList() = 0;
                virtual const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetBaseNodeSelectionList() const = 0 ;
            };
        }
    }
}