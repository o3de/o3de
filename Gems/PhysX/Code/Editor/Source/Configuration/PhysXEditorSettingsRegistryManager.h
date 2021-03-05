/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
