#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
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
            class ISkinGroup
                : public ISceneNodeGroup
            {
            public:
                AZ_RTTI(ISkinGroup, "{D3FD3067-0291-4274-8C58-050B47095747}", ISceneNodeGroup);

                ~ISkinGroup() override = default;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
