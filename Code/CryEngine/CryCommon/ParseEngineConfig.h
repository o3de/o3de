/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include "ISystem.h"
#include <AzCore/base.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Platform/PlatformDefaults.h>

// any of the following tags can be present in the bootstrap.cfg
// you can also prefix it with a platform.
// so for example, you can specify remote_ip alone to specify it for all platforms
// or you could specify android_remote_ip to change it for android only.
// the instructions are executed in the order that they appear, so you can set the default
// by using the non-platform-specific version, and then later on in the file you
// can override specific platforms.

#define CONFIG_KEY_FOR_REMOTEIP                 "remote_ip"
#define CONFIG_KEY_FOR_REMOTEPORT               "remote_port"
#define CONFIG_KEY_FOR_GAMEFOLDER               "sys_game_folder"
#define CONFIG_KEY_FOR_REMOTEFILEIO             "remote_filesystem"
#define CONFIG_KEY_FOR_CONNECTTOREMOTE          "connect_to_remote"
#define CONFIG_KEY_WAIT_FOR_CONNECT             "wait_for_connect"
#define DEFAULT_GAMEDLL                         "EmptyTemplate"
#define DEFAULT_GAMEFOLDER                      "EmptyTemplate"
#define DEFAULT_REMOTEIP                        "127.0.0.1"
#define DEFAULT_REMOTEPORT                      45643
#define CONFIG_KEY_FOR_ASSETS                   "assets"
#define CONFIG_KEY_FOR_BRANCHTOKEN              "assetProcessor_branch_token"

//////////////////////////////////////////////////////////////////////////
class CEngineConfig
{
public:
    string m_gameFolder; // folder only ("MyGame")
    string m_assetPlatform; // what platform folder assets are from if more than one is available or using VFS ("pc" / "es3")
    bool   m_connectToRemote;
    bool   m_remoteFileIO;
    bool   m_waitForConnect;
    string m_remoteIP;
    int    m_remotePort;

    string m_rootFolder; // The engine root folder
    string m_branchToken;

    CEngineConfig([[maybe_unused]] const char** sourcePaths = nullptr, [[maybe_unused]] size_t numSearchPaths = 0, [[maybe_unused]] size_t numLevelsUp = 3)
        : m_gameFolder(DEFAULT_GAMEFOLDER)
        , m_connectToRemote(false)
        , m_remoteFileIO(false)
        , m_remotePort(DEFAULT_REMOTEPORT)
        , m_waitForConnect(false)
        , m_remoteIP(DEFAULT_REMOTEIP)
    {
        m_assetPlatform = AzFramework::OSPlatformToDefaultAssetPlatform(AZ_TRAIT_OS_PLATFORM_CODENAME);

        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            AZ::SettingsRegistryInterface::FixedValueString gameFolder;
            auto gameFolderKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/%s",
                AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, CONFIG_KEY_FOR_GAMEFOLDER);
            if (settingsRegistry->Get(gameFolder, gameFolderKey))
            {
                m_gameFolder.assign(gameFolder.c_str(), gameFolder.size());
            }

            AZ::SettingsRegistryInterface::FixedValueString engineRoot;
            if (settingsRegistry->Get(engineRoot, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
            {
                m_rootFolder.assign(engineRoot.c_str(), engineRoot.size());
            }
        }

        OnLoadSettings();
    }

    void CopyToStartupParams(SSystemInitParams& startupParams) const
    {
        startupParams.remoteFileIO = m_remoteFileIO;
        startupParams.remotePort = m_remotePort;
        startupParams.connectToRemote = m_connectToRemote;
        startupParams.waitForConnection = m_waitForConnect;

        azstrncpy(startupParams.remoteIP, sizeof(startupParams.remoteIP), m_remoteIP.c_str(), m_remoteIP.length() + 1); // +1 for the null terminator
        azstrncpy(startupParams.assetsPlatform, sizeof(startupParams.assetsPlatform), m_assetPlatform.c_str(), m_assetPlatform.length() + 1); // +1 for the null terminator
        azstrncpy(startupParams.rootPath, sizeof(startupParams.rootPath), m_rootFolder.c_str(), m_rootFolder.length() + 1); // +1 for the null terminator
        azstrncpy(startupParams.gameFolderName, sizeof(startupParams.gameFolderName), m_gameFolder.c_str(), m_gameFolder.length() + 1); // +1 for the null terminator
        azstrncpy(startupParams.branchToken, sizeof(startupParams.branchToken), m_branchToken.c_str(), m_branchToken.length() + 1); // +1 for the null terminator

        // compute assets path based on game folder name
        string gameFolderLower(m_gameFolder);
        gameFolderLower.MakeLower();
        azsnprintf(startupParams.assetsPath, sizeof(startupParams.assetsPath), "%s/%s", startupParams.rootPath, gameFolderLower.c_str());

        // compute where the cache should be located
        azsnprintf(startupParams.rootPathCache, sizeof(startupParams.rootPathCache), "%s/Cache/%s/%s", m_rootFolder.c_str(), m_gameFolder.c_str(), m_assetPlatform.c_str());
        azsnprintf(startupParams.assetsPathCache, sizeof(startupParams.assetsPathCache), "%s/%s", startupParams.rootPathCache, gameFolderLower.c_str());
    }

protected:

    void OnLoadSettings()
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Warning("ParseEngineConfig", false, "Attempting to load configuration data while SettingsRegistry does not exist");
            return;

        }
        AZ::SettingsRegistryInterface::FixedValueString settingsKeyPrefix = AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey;
        AZ::SettingsRegistryInterface::FixedValueString settingsValueString;
        AZ::s64 settingsValueInt{};

        if (AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, settingsValueInt, settingsKeyPrefix, CONFIG_KEY_FOR_REMOTEFILEIO))
        {
            m_remoteFileIO = settingsValueInt != 0;
        }
        if (AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, settingsValueInt, settingsKeyPrefix, CONFIG_KEY_WAIT_FOR_CONNECT))
        {
            m_waitForConnect = settingsValueInt != 0;
        }
        if (AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, settingsValueInt, settingsKeyPrefix, CONFIG_KEY_FOR_CONNECTTOREMOTE))
        {
            m_connectToRemote = settingsValueInt != 0;
        }
        if (AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, settingsValueInt, settingsKeyPrefix, CONFIG_KEY_FOR_REMOTEPORT))
        {
            m_remotePort = aznumeric_cast<AZ::u16>(settingsValueInt);
        }
        if (settingsValueString = {};
            AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, settingsValueString, settingsKeyPrefix, CONFIG_KEY_FOR_REMOTEIP))
        {
            m_remoteIP.assign(settingsValueString.c_str(), settingsValueString.size());
        }
        if (settingsValueString = {};
            AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, settingsValueString, settingsKeyPrefix, CONFIG_KEY_FOR_ASSETS))
        {
            m_assetPlatform.assign(settingsValueString.c_str(), settingsValueString.size());
        }
        if (settingsValueString = {};
            settingsRegistry->Get(settingsValueString, settingsKeyPrefix + "/" + CONFIG_KEY_FOR_BRANCHTOKEN))
        {
            m_branchToken.assign(settingsValueString.c_str(), settingsValueString.size());
        }
        
    }
};
