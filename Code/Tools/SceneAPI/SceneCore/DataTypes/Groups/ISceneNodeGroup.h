#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
