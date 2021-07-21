/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Streamer/StorageDrive_Windows.h>
#include <AzCore/IO/Streamer/StorageDriveConfig_Windows.h>
#include <AzCore/IO/Streamer/StreamerConfiguration_Windows.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::IO
{
    AZStd::shared_ptr<StreamStackEntry> WindowsStorageDriveConfig::AddStreamStackEntry(
        const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent)
    {
        const DriveList* drives = AZStd::any_cast<DriveList>(&hardware.m_platformData);

        if (drives && !drives->empty())
        {
            for (const DriveInformation& drive : *drives)
            {
                StorageDriveWin::ConstructionOptions options;
                options.m_enableUnbufferedReads = m_enableUnbufferedReads;
                options.m_enableSharing = m_enableFileSharing;
                options.m_hasSeekPenalty = drive.m_hasSeekPenalty;
                options.m_minimalReporting = m_minimalReporting;

                AZStd::vector<AZStd::string_view> drivePaths(drive.m_paths.begin(), drive.m_paths.end());
                AZ_Assert(!drive.m_paths.empty(), "Expected at least one drive path.");
                auto stackEntry = AZStd::make_shared<StorageDriveWin>(
                    AZStd::move(drivePaths), m_maxFileHandles, m_maxMetaDataCache, drive.m_physicalSectorSize, drive.m_logicalSectorSize,
                    drive.m_ioChannelCount, m_overcommit, options);

                stackEntry->SetNext(AZStd::move(parent));
                parent = stackEntry;
            }
        }
        else
        {
            AZ_Warning("Streamer", false, "No drives found that can make use of the available optimizations.\n");
        }
        return parent;
    }

    void WindowsStorageDriveConfig::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<WindowsStorageDriveConfig, IStreamerStackConfig>()
                ->Version(1)
                ->Field("MaxFileHandles", &WindowsStorageDriveConfig::m_maxFileHandles)
                ->Field("MaxMetaDataCache", &WindowsStorageDriveConfig::m_maxMetaDataCache)
                ->Field("Overcommit", &WindowsStorageDriveConfig::m_overcommit)
                ->Field("EnableFileSharing", &WindowsStorageDriveConfig::m_enableFileSharing)
                ->Field("EnableUnbufferedReads", &WindowsStorageDriveConfig::m_enableUnbufferedReads)
                ->Field("MinimalReporting", &WindowsStorageDriveConfig::m_minimalReporting);
        }
    }
} // namespace AZ::IO
