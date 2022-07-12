/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/Factory.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/CommandLine/CommandLine.h>

static constexpr char GraphicsDevModeSetting[] = "/O3DE/Atom/RHI/GraphicsDevMode";

namespace AZ
{
    namespace RHI
    {
        Ptr<Device> GetRHIDevice()
        {
            RHISystemInterface* rhiSystem = RHISystemInterface::Get();
            AZ_Assert(rhiSystem, "Failed to retrieve rpi system.");
            return rhiSystem->GetDevice();
        }

        FormatCapabilities GetCapabilities(ScopeAttachmentUsage scopeUsage, ScopeAttachmentAccess scopeAccess, AttachmentType attachmentType)
        {
            scopeAccess = AdjustAccessBasedOnUsage(scopeAccess, scopeUsage);

            FormatCapabilities capabilities = FormatCapabilities::None;

            if (attachmentType == AttachmentType::Image)
            {
                if (scopeUsage == ScopeAttachmentUsage::RenderTarget)
                {
                    capabilities |= FormatCapabilities::RenderTarget;
                }
                else if (scopeUsage == ScopeAttachmentUsage::DepthStencil)
                {
                    capabilities |= FormatCapabilities::DepthStencil;
                }
                else if (scopeUsage == ScopeAttachmentUsage::Shader)
                {
                    capabilities |= FormatCapabilities::Sample;

                    if (scopeAccess == ScopeAttachmentAccess::Write)
                    {
                        capabilities |= FormatCapabilities::TypedStoreBuffer;
                    }
                }
            }
            else if (attachmentType == AttachmentType::Buffer)
            {
                if (scopeAccess == ScopeAttachmentAccess::Read)
                {
                    capabilities |= FormatCapabilities::TypedLoadBuffer;
                }
                if (scopeAccess == ScopeAttachmentAccess::Write)
                {
                    capabilities |= FormatCapabilities::TypedStoreBuffer;
                }
            }

            return capabilities;
        }

        Format GetNearestDeviceSupportedFormat(Format requestedFormat, FormatCapabilities requestedCapabilities)
        {
            Ptr<Device> device = GetRHIDevice();
            if (device)
            {
                return device->GetNearestSupportedFormat(requestedFormat, requestedCapabilities);
            }
            AZ_Assert(device, "Failed to retrieve device.");
            return Format::Unknown;
        }

        Format ValidateFormat(Format originalFormat, [[maybe_unused]] const char* location, const AZStd::vector<Format>& formatFallbacks, FormatCapabilities requestedCapabilities)
        {
            if (originalFormat != Format::Unknown)
            {
                Format format = originalFormat;
                Format nearestFormat = GetNearestDeviceSupportedFormat(format, requestedCapabilities);

                // If format not supported, check all the fallbacks in the list provided
                for (size_t i = 0; i < formatFallbacks.size() && format != nearestFormat; ++i)
                {
                    format = formatFallbacks[i];
                    nearestFormat = GetNearestDeviceSupportedFormat(format, requestedCapabilities);
                }

                if (format != nearestFormat)
                {
                    // Fallback to format that's closest to the original
                    nearestFormat = GetNearestDeviceSupportedFormat(format, requestedCapabilities);

                    AZ_Warning("RHI Utils", format == nearestFormat, "%s specifies format [%s], which is not supported by this device. Overriding to nearest supported format [%s]",
                        location, ToString(format), ToString(nearestFormat));
                }
                return nearestFormat;
            }
            return originalFormat;
        }
        
        bool IsNullRHI()
        {
            return RHI::Factory::Get().GetAPIUniqueIndex() == static_cast<uint32_t>(APIIndex::Null);
        }

        AZStd::string GetCommandLineValue(const AZStd::string& commandLineOption)
        {
            const AzFramework::CommandLine* commandLine = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);

            AZStd::string commandLineValue;
            if (commandLine)
            {
                if (size_t switchCount = commandLine->GetNumSwitchValues(commandLineOption); switchCount > 0)
                {
                    commandLineValue = commandLine->GetSwitchValue(commandLineOption, switchCount - 1);
                }
            }
            return commandLineValue;
        }

        bool QueryCommandLineOption(const AZStd::string& commandLineOption)
        {
            const AzFramework::CommandLine* commandLine = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);

            if (commandLine)
            {
                return commandLine->HasSwitch(commandLineOption);
            }
            return false;
        }

        bool IsGraphicsDevModeEnabled()
        {
            AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
            bool graphicsDevMode = false;
            if (settingsRegistry)
            {
                settingsRegistry->Get(graphicsDevMode, GraphicsDevModeSetting);
            }
            return graphicsDevMode;
        }
    }
}
