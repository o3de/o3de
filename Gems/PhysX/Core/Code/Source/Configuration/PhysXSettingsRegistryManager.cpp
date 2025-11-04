/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Configuration/PhysXSettingsRegistryManager.h>

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>

namespace PhysX
{

#if !defined(PHYSX_SETREG_GEM_NAME)
    #error "Missing required PHYSX_SETREG_GEM_NAME definition"
#endif //!defined(PHYSX_SETREG_GEM_NAME)

    PhysXSettingsRegistryManager::PhysXSettingsRegistryManager()
    {
        m_settingsRegistryPath = AZStd::string::format("%s/Gems/" PHYSX_SETREG_GEM_NAME "/PhysXSystemConfiguration", AZ::SettingsRegistryMergeUtils::OrganizationRootKey);
        m_defaultSceneConfigSettingsRegistryPath = AZStd::string::format("%s/Gems/" PHYSX_SETREG_GEM_NAME "/DefaultSceneConfiguration", AZ::SettingsRegistryMergeUtils::OrganizationRootKey);
        m_debugSettingsRegistryPath = AZStd::string::format("%s/Gems/" PHYSX_SETREG_GEM_NAME "/Debug/PhysXDebugConfiguration", AZ::SettingsRegistryMergeUtils::OrganizationRootKey);
    }

    AZStd::optional<PhysXSystemConfiguration> PhysXSettingsRegistryManager::LoadSystemConfiguration() const
    {
        PhysXSystemConfiguration systemConfig;

        bool configurationRead = false;
        
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            configurationRead = settingsRegistry->GetObject(systemConfig, m_settingsRegistryPath);
        }

        if (configurationRead)
        {
            AZ_TracePrintf("PhysXSystem", R"(PhysXConfiguration was read from settings registry at pointer path)"
                R"( "%s)" "\n",
                m_settingsRegistryPath.c_str());
            return systemConfig;
        }
        return AZStd::nullopt;
    }

    AZStd::optional<AzPhysics::SceneConfiguration> PhysXSettingsRegistryManager::LoadDefaultSceneConfiguration() const
    {
        AzPhysics::SceneConfiguration sceneConfig;
        bool configurationRead = false;

        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            configurationRead = settingsRegistry->GetObject(sceneConfig, m_defaultSceneConfigSettingsRegistryPath);
        }

        if (configurationRead)
        {
            AZ_TracePrintf("PhysXSystem", R"(Default Scene Configuration was read from settings registry at pointer path)"
                R"("%s)" "\n",
                m_defaultSceneConfigSettingsRegistryPath.c_str());
            return sceneConfig;
        }
        return AZStd::nullopt;
    }

    AZStd::optional<Debug::DebugConfiguration> PhysXSettingsRegistryManager::LoadDebugConfiguration() const
    {
        Debug::DebugConfiguration systemConfig;

        bool configurationRead = false;

        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            configurationRead = settingsRegistry->GetObject(systemConfig, m_debugSettingsRegistryPath);
        }

        if (configurationRead)
        {
            AZ_TracePrintf("PhysXSystem", R"(Debug::DebugConfiguration was read from settings registry at pointer path)"
                R"( "%s)" "\n",
                m_debugSettingsRegistryPath.c_str());
            return systemConfig;
        }
        return AZStd::nullopt;
    }

    void PhysXSettingsRegistryManager::SaveSystemConfiguration([[maybe_unused]] const PhysXSystemConfiguration& config, const OnPhysXConfigSaveComplete& saveCallback) const
    {
        //PhysXEditorSettingsRegistryManager will implement, as saving is editor only currently.
        if (saveCallback)
        {
            saveCallback(config, Result::Failed);
        } 
    }

    void PhysXSettingsRegistryManager::SaveDefaultSceneConfiguration([[maybe_unused]] const AzPhysics::SceneConfiguration& config, const OnDefaultSceneConfigSaveComplete& saveCallback) const
    {
        //PhysXEditorSettingsRegistryManager will implement, as saving is editor only currently.
        if (saveCallback)
        {
            saveCallback(config, Result::Failed);
        }
    }

    void PhysXSettingsRegistryManager::SaveDebugConfiguration([[maybe_unused]] const Debug::DebugConfiguration& config, const OnPhysXDebugConfigSaveComplete& saveCallback) const
    {
        //PhysXEditorSettingsRegistryManager will implement, as saving is editor only currently.
        if (saveCallback)
        {
            saveCallback(config, Result::Failed);
        }
    }
} //namespace PhysX

