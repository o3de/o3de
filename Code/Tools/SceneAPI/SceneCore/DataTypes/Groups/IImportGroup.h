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

        //! Settings registry key that contains the default values to use for import settings.
        //! These can either be set in a setreg file or programmatically by saving into the current settings registry.
        //! These settings will be overridden by any settings that exist in the .assetinfo file.
        static inline constexpr const char SceneImportSettingsRegistryKey[] = "/O3DE/Preferences/SceneAPI/ImportSettings";

        //! Get the import settings stored in the Import Group.
        virtual const SceneImportSettings& GetImportSettings() const = 0;

        //! Set the import settings in this Import Group.
        virtual void SetImportSettings(const SceneImportSettings& importSettings) = 0;
    };
}
