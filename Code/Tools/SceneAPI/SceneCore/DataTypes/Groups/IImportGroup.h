/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneCore/Import/SceneImportSettings.h>

namespace AZ::SceneAPI::DataTypes
{
    class IImportGroup
        : public ISceneNodeGroup
    {
    public:
        AZ_RTTI(IImportGroup, "{CF09F740-5A5A-4865-A9A7-FAD1A70C045E}", ISceneNodeGroup);

        ~IImportGroup() override = default;

        virtual const SceneImportSettings& GetImportSettings() const = 0;
    };
}
