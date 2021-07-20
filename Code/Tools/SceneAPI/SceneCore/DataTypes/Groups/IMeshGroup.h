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
            class IMeshGroup
                : public ISceneNodeGroup
            {
            public:
                AZ_RTTI(IMeshGroup, "{74D45E45-81EE-4AD4-83B5-F37EB98D847C}", ISceneNodeGroup);

                ~IMeshGroup() override = default;
                virtual void SetName(AZStd::string&& name) = 0;
                virtual void OverrideId(const Uuid& id) = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
