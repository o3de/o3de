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

#include <memory>
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class DataObject;
        }
        namespace DataTypes
        {
            class ISceneNodeGroup
                : public IGroup
            {
            public:
                AZ_RTTI(ISceneNodeGroup, "{1D20FA11-B184-429E-8C86-745852234845}", IGroup);

                ~ISceneNodeGroup() override = default;

                virtual DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() = 0;
                virtual const DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
