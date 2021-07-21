/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/Path/Path.h>

#include <Configuration/PhysXSettingsRegistryManager.h>

namespace PhysX
{
    //! Handles loading and saving the settings registry
    class PhysXEditorSettingsRegistryManager : public PhysXSettingsRegistryManager
    {
    public:
        PhysXEditorSettingsRegistryManager();

        // PhysXSystemSettingsRegistry ...
        void SaveSystemConfiguration(const PhysXSystemConfiguration& config, const OnPhysXConfigSaveComplete& saveCallback) const override;
        void SaveDefaultSceneConfiguration(const AzPhysics::SceneConfiguration& config, const OnDefaultSceneConfigSaveComplete& saveCallback) const override;
        void SaveDebugConfiguration(const Debug::DebugConfiguration& config, const OnPhysXDebugConfigSaveComplete& saveCallback) const override;

    private:
        AZ::IO::FixedMaxPath m_physXConfigurationFilePath = "Registry/physxsystemconfiguration.setreg";
        AZ::IO::FixedMaxPath m_defaultSceneConfigFilePath = "Registry/physxdefaultsceneconfiguration.setreg";
        AZ::IO::FixedMaxPath m_debugConfigurationFilePath = "Registry/physxdebugconfiguration.setreg";

        bool m_initialized = false;
    };
}
