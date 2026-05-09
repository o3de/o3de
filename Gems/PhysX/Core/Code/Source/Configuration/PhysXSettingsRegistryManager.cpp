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
    PhysXSettingsRegistryManager::PhysXSettingsRegistryManager()
    {
        const char* physXGemNames[] = { "PhysX5", "PhysX" };
        for (const char* gemName : physXGemNames) 
        {
            m_settingsRegistryPath.push_back(AZStd::string::format("%s/Gems/%s/PhysXSystemConfiguration", AZ::SettingsRegistryMergeUtils::OrganizationRootKey, gemName));
            m_defaultSceneConfigSettingsRegistryPath.push_back(AZStd::string::format("%s/Gems/%s/DefaultSceneConfiguration", AZ::SettingsRegistryMergeUtils::OrganizationRootKey, gemName));
            m_debugSettingsRegistryPath.push_back(AZStd::string::format("%s/Gems/%s/Debug/PhysXDebugConfiguration", AZ::SettingsRegistryMergeUtils::OrganizationRootKey, gemName));
        }
    }

    AZStd::optional<PhysXSystemConfiguration> PhysXSettingsRegistryManager::LoadSystemConfiguration() const
    {
        PhysXSystemConfiguration systemConfig;

        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            for (const auto& path : m_settingsRegistryPath) 
            {
                if (settingsRegistry->GetObject(systemConfig, path))
                {
                    AZ_Printf("PhysXSystem", R"(PhysXConfiguration was read from settings registry at pointer path)"
                                             R"( "%s)" "\n",
                                             path.c_str());
                    return systemConfig;
                }
            }
        }
        return AZStd::nullopt;
    }

    AZStd::optional<AzPhysics::SceneConfiguration> PhysXSettingsRegistryManager::LoadDefaultSceneConfiguration() const
    {
        AzPhysics::SceneConfiguration sceneConfig;

        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            for (const auto& path : m_defaultSceneConfigSettingsRegistryPath)
            {
                if (settingsRegistry->GetObject(sceneConfig, path))
                {
                    AZ_Printf("PhysXSystem", R"(Default Scene Configuration was read from settings registry at pointer path)"
                                             R"( "%s)" "\n",
                                             path.c_str());
                    return sceneConfig;
                }
            }
        }

        return AZStd::nullopt;
    }

    AZStd::optional<Debug::DebugConfiguration> PhysXSettingsRegistryManager::LoadDebugConfiguration() const
    {
        Debug::DebugConfiguration systemConfig;

        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            for (const auto& path : m_debugSettingsRegistryPath)
            {
                if (settingsRegistry->GetObject(systemConfig, path))
                {
                    AZ_Printf("PhysXSystem", R"(Debug::DebugConfiguration was read from settings registry at pointer path)"
                                             R"( "%s)" "\n",
                                             path.c_str());
                    return systemConfig;
                }
            }
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

