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

#include <AzCore/std/optional.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <PhysX/Configuration/PhysXConfiguration.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>

namespace PhysX
{
    //! Handles loading and saving the settings registry
    class PhysXSettingsRegistryManager
    {
    public:
        enum class Result : AZ::u8
        {
            Success,
            Failed
        };
        using OnPhysXConfigSaveComplete = AZStd::function<void(const PhysXSystemConfiguration&, Result)>;
        using OnDefaultSceneConfigSaveComplete = AZStd::function<void(const AzPhysics::SceneConfiguration&, Result)>;
        using OnPhysXDebugConfigSaveComplete = AZStd::function<void(const Debug::DebugConfiguration&, Result)>;

        PhysXSettingsRegistryManager();
        virtual ~PhysXSettingsRegistryManager() = default;

        //! Load the PhysX Configuration from the Settings Registry
        //! @return Returns true if successful.
        virtual AZStd::optional<PhysXSystemConfiguration> LoadSystemConfiguration() const;

        //! Load the Default Scene Configuration from the Settings Registry
        //! @return Returns true if successful.
        virtual AZStd::optional<AzPhysics::SceneConfiguration> LoadDefaultSceneConfiguration() const;

        //! Load the PhysX Debug Configuration from the Settings Registry
        //! @return Returns true if successful.
        virtual AZStd::optional<Debug::DebugConfiguration> LoadDebugConfiguration() const;

        //! Save the PhysX Configuration from the Settings Registry
        //! @return Returns true if successful. When not in Editor, always returns false.
        virtual void SaveSystemConfiguration(const PhysXSystemConfiguration& config, const OnPhysXConfigSaveComplete& saveCallback) const;

        //! Save the Default Scene Configuration  from the Settings Registry
        //! @return Returns true if successful. When not in Editor, always returns false.
        virtual void SaveDefaultSceneConfiguration(const AzPhysics::SceneConfiguration& config, const OnDefaultSceneConfigSaveComplete& saveCallback) const;

        //! Save the PhysX Debug Configuration from the Settings Registry
        //! @return Returns true if successful. When not in Editor, always returns false.
        virtual void SaveDebugConfiguration(const Debug::DebugConfiguration& config, const OnPhysXDebugConfigSaveComplete& saveCallback) const;

    protected:
        AZStd::string m_settingsRegistryPath;
        AZStd::string m_defaultSceneConfigSettingsRegistryPath;
        AZStd::string m_debugSettingsRegistryPath;
    };
}
