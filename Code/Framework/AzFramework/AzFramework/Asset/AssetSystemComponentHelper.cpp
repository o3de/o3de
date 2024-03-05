/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzFramework/Platform/PlatformDefaults.h>

namespace AzFramework::AssetSystem::Platform
{
    // Declare platform specific AllowAssetProcessorToForeground function
    void AllowAssetProcessorToForeground();

    // Declare platform specific LaunchAssetProcessor function
    bool LaunchAssetProcessor(AZStd::string_view executableDirectory, AZStd::string_view appRoot,
        AZStd::string_view projectPath);
}

namespace AzFramework
{
    namespace AssetSystem
    {
        void AllowAssetProcessorToForeground()
        {
            Platform::AllowAssetProcessorToForeground();
        }

        // Do this here because including Windows.h causes problems with SetPort being a define
        void AssetSystemComponent::ShowAssetProcessor()
        {
            AllowAssetProcessorToForeground();

            ShowAssetProcessorRequest request;
            SendRequest(request);
        }

        void AssetSystemComponent::UpdateSourceControlStatus(bool newStatus)
        {
            UpdateSourceControlStatusRequest request;
            request.m_sourceControlEnabled = newStatus;
            SendRequest(request);
        }

        void AssetSystemComponent::ShowInAssetProcessor(const AZStd::string& assetPath)
        {
            AllowAssetProcessorToForeground();

            ShowAssetInAssetProcessorRequest request;
            request.m_assetPath = assetPath;
            SendRequest(request);
        }
        
        bool LaunchAssetProcessor()
        {
            AZ::IO::FixedMaxPathString executableDirectory;
            if (AZ::Utils::GetExecutableDirectory(executableDirectory.data(), executableDirectory.max_size()) == AZ::Utils::ExecutablePathResult::Success)
            {
                // Update the size member of the FixedString stored in the path class
                executableDirectory.resize_no_construct(AZStd::char_traits<char>::length(executableDirectory.data()));
            }

            auto settingsRegistry = AZ::SettingsRegistry::Get();

            // Add the engine path to the launch command if available from the Settings Registry
            AZ::SettingsRegistryInterface::FixedValueString engineRootFolder;
            if (settingsRegistry)
            {
                settingsRegistry->Get(engineRootFolder, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            }

            // Add the active project's path to the launch command if available from the Settings Registry
            AZ::SettingsRegistryInterface::FixedValueString projectPath;
            if (settingsRegistry)
            {
                settingsRegistry->Get(projectPath, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
            }

            if (!Platform::LaunchAssetProcessor(executableDirectory, engineRootFolder, projectPath))
            {
                // if we are unable to launch asset processor
                AzFramework::AssetSystemInfoBus::Broadcast(&AzFramework::AssetSystem::AssetSystemInfoNotifications::OnError, AssetSystemErrors::ASSETSYSTEM_FAILED_TO_LAUNCH_ASSETPROCESSOR);
                return false;
            }

            return true;
        }

        bool ReadConnectionSettingsFromSettingsRegistry(ConnectionSettings& outputConnectionSettings)
        {
            bool result = true;
            AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
            {
                // Read Asset Processor IP Address from Settings Registry
                AZ::SettingsRegistryInterface::FixedValueString ip;
                if (!AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, ip,
                    AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, AzFramework::AssetSystem::AssetProcessorRemoteIp))
                {
                    AZ_TraceOnce("AssetSystemComponent", "Failed to find ip, setting 127.0.0.1\n");
                    outputConnectionSettings.m_assetProcessorIp = "127.0.0.1";
                }
                else if (ip.empty())
                {
                    AZ_TraceOnce("AssetSystemComponent", "Ip is empty, setting 127.0.0.1\n");
                    outputConnectionSettings.m_assetProcessorIp = "127.0.0.1";
                }
                else
                {
                    //check the ip for obvious things wrong
                    size_t iplen = ip.length();
                    int countseperators = 0;
                    bool isNumeric = true;
#if AZ_TRAIT_DENY_ASSETPROCESSOR_LOOPBACK
                    bool isIllegalLoopBack = ip == "127.0.0.1";
#endif
                    for (int i = 0; isNumeric && i < iplen; ++i)
                    {
                        if (ip[i] == '.')
                        {
                            countseperators++;
                        }
                        else if (!isdigit(ip[i]))
                        {
                            isNumeric = false;
                        }
                    }

                    if (iplen < 7 ||
                        countseperators != 3 ||
#if AZ_TRAIT_DENY_ASSETPROCESSOR_LOOPBACK
                        isIllegalLoopBack ||
#endif
                        !isNumeric)
                    {
                        AZ_Error("AssetSystemComponent", false, "IP address of the Asset Processor is invalid!\nMake sure the remote_ip in the bootstrap.cfg is correct.\n");
                        result = false;
                    }

                    outputConnectionSettings.m_assetProcessorIp = ip;
                }
            }

            {
                // Read AssetProcessor port from Settings Registry
                AZ::s64 port64;
                if (!AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, port64, AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, AzFramework::AssetSystem::AssetProcessorRemotePort))
                {
                    AZ_TraceOnce("AssetSystemComponent", "Failed to find port, setting 45643\n");
                    outputConnectionSettings.m_assetProcessorPort = 45643;
                }
                else
                {
                    outputConnectionSettings.m_assetProcessorPort = aznumeric_cast<AZ::u16>(port64);
                }
            }

            {
                // Read the Asset Platform from the Settings Registry
                AZ::SettingsRegistryInterface::FixedValueString assetsPlatform;
                if (!AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, assetsPlatform, AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, AzFramework::AssetSystem::Assets))
                {
                    AZ_TraceOnce("AssetSystemComponent", "Failed to find asset platform, setting 'pc'\n");
                    outputConnectionSettings.m_assetPlatform = "pc";
                }
                else
                {
                    outputConnectionSettings.m_assetPlatform = assetsPlatform;
                }

                if (outputConnectionSettings.m_assetPlatform.empty())
                {
                    assetsPlatform = AzFramework::OSPlatformToDefaultAssetPlatform(AZ_TRAIT_OS_PLATFORM_CODENAME);
                    AZ_TraceOnce("AssetSystemComponent", "Asset platform read from bootstrap is empty, setting %s\n", assetsPlatform.c_str());
                }
            }

            {
                // Read Branch Token from Settings Registry
                AZStd::string branchToken;
                if (!settingsRegistry->Get(branchToken, AZ::SettingsRegistryInterface::FixedValueString::format("%s/%s",
                    AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, AzFramework::AssetSystem::BranchToken)))
                {
                    // The first time the AssetProcessor runs within a branch the bootstrap.cfg does not have a branch token set
                    // Therefore it is not an error for the branch token to not be in the bootstrap.cfg file
                    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::CalculateBranchTokenForEngineRoot, branchToken);
                    AZ_TraceOnce("AssetSystemComponent", "Failed to read branch token from bootstrap. Calculating Branch Token: %s\n", branchToken.c_str());
                    outputConnectionSettings.m_branchToken = branchToken;
                }
                else
                {
                    outputConnectionSettings.m_branchToken = AZStd::string_view{ branchToken };
                    if (outputConnectionSettings.m_branchToken.empty())
                    {
                        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::CalculateBranchTokenForEngineRoot, branchToken);
                        AZ_TraceOnce("AssetSystemComponent", "Branch token read from bootstrap is empty. Calculating Branch Token: %s\n", branchToken.c_str());
                        outputConnectionSettings.m_branchToken = branchToken;
                    }
                }
            }

            {
                // Read Project Name from Settings Registry
                AZ::SettingsRegistryInterface::FixedValueString projectName = AZ::Utils::GetProjectName();
                if (projectName.empty())
                {
                    AZ_Error("AssetSystemComponent", false, "Failed to read project name from registry");
                    result = false;
                }

                outputConnectionSettings.m_projectName = projectName;
            }

            // Read the direction in which a connection to the Asset Processor should be from using the Settings Registry
            // Determine whether to connect to the Asset Processor or whether the AssetProcessor
            // connects to the game "listen" socket
            AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, reinterpret_cast<AZ::s64&>(outputConnectionSettings.m_connectionDirection),
                AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "connect_to_remote");

            {
                // Read the wait for connection boolean from the Settings Registry
                AZ::s64 waitForConnect64{};
                if (AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, waitForConnect64, AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, AzFramework::AssetSystem::WaitForConnect))
                {
                    outputConnectionSettings.m_waitForConnect = waitForConnect64 != 0;
                }
            }

            // Read timeout values from the Settings Registry
            AZ::s64 timeoutValue{};
            if (AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, timeoutValue,
                AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "connect_ap_timeout"))
            {
                outputConnectionSettings.m_connectTimeout = AZStd::chrono::seconds(timeoutValue);
            }

            // Reset timeout integer
            timeoutValue = {};
            if (AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, timeoutValue,
                AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "launch_ap_timeout"))
            {
                outputConnectionSettings.m_launchTimeout = AZStd::chrono::seconds(timeoutValue);
            }

            timeoutValue = {};
            if (AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, timeoutValue,
                AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "wait_ap_ready_timeout"))
            {
                outputConnectionSettings.m_waitForReadyTimeout = AZStd::chrono::seconds(timeoutValue);
            }

            return result;
        }
    }
}


