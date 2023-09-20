/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/IStreamerTypes.h>
#include <AzCore/IO/Streamer/StorageDriveConfig_Windows.h>
#include <AzCore/IO/Streamer/StreamerConfiguration_Windows.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/StringFunc/StringFunc.h>

// https://developercommunity.visualstudio.com/t/windows-sdk-100177630-pragma-push-pop-mismatch-in/386142
#define _NTDDSCM_H_
#include <winioctl.h>

namespace AZ::IO
{
    static DWORD NextPowerOfTwo(DWORD value)
    {
        --value;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        ++value;
        return value;
    }

    static void CollectIoAdaptor(HANDLE deviceHandle, DriveInformation& info, [[maybe_unused]] const char* driveName, bool reportHardware)
    {
        STORAGE_ADAPTER_DESCRIPTOR adapterDescriptor{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageAdapterProperty;

        DWORD bytesReturned = 0;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &adapterDescriptor, sizeof(adapterDescriptor), &bytesReturned, nullptr))
        {
            switch (adapterDescriptor.BusType)
            {
            case BusTypeScsi:               info.m_profile = "Scsi";                break;
            case BusTypeAtapi:              info.m_profile = "Atapi";               break;
            case BusTypeAta:                info.m_profile = "Ata";                 break;
            case BusType1394:               info.m_profile = "1394";                break;
            case BusTypeSsa:                info.m_profile = "Ssa";                 break;
            case BusTypeFibre:              info.m_profile = "Fibre";               break;
            case BusTypeUsb:                info.m_profile = "Usb";                 break;
            case BusTypeRAID:               info.m_profile = "Raid";                break;
            case BusTypeiScsi:              info.m_profile = "iScsi";               break;
            case BusTypeSas:                info.m_profile = "Sas";                 break;
            case BusTypeSata:               info.m_profile = "Sata";                break;
            case BusTypeSd:                 info.m_profile = "Sd";                  break;
            case BusTypeMmc:                info.m_profile = "Mmc";                 break;
            case BusTypeVirtual:            info.m_profile = "Virtual";             break;
            case BusTypeFileBackedVirtual:  info.m_profile = "File backed virtual"; break;
            case BusTypeSpaces:             info.m_profile = "Spaces";              break;
            case BusTypeNvme:               info.m_profile = "Nvme";                break;
            case BusTypeSCM:                info.m_profile = "SCM";                 break;
            case BusTypeUfs:                info.m_profile = "Ufs";                 break;
            default:                        info.m_profile = "Generic";
            }

            DWORD pageSize = NextPowerOfTwo(adapterDescriptor.MaximumTransferLength / adapterDescriptor.MaximumPhysicalPages);

            if (reportHardware)
            {
                AZ_Trace(
                    "Streamer",
                    "Adapter for drive '%s':\n"
                    "    Bus: %s %i.%i\n"
                    "    Max transfer: %.3f kb\n"
                    "    Page size: %i kb for %i pages\n"
                    "    Supports queuing: %s\n",
                    driveName,
                    info.m_profile.c_str(), adapterDescriptor.BusMajorVersion, adapterDescriptor.BusMinorVersion,
                    (1.0f / 1024.0f) * adapterDescriptor.MaximumTransferLength,
                    pageSize, adapterDescriptor.MaximumPhysicalPages,
                    adapterDescriptor.CommandQueueing ? "Yes" : "No");
            }

            info.m_maxTransfer = adapterDescriptor.MaximumTransferLength;
            info.m_pageSize = pageSize;
            info.m_supportsQueuing = adapterDescriptor.CommandQueueing;
        }
    }

    static void AppendDriveTypeToProfile(HANDLE deviceHandle, DriveInformation& information, bool reportHardware)
    {
        // Check for support for the TRIM command. This command is exclusively used by SSD drives so can be used to
        // tell the difference between SSD and HDD. Since these are the only 2 types supported right now, no further
        // checks need to happen.

        DEVICE_TRIM_DESCRIPTOR trimDescriptor{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageDeviceTrimProperty;

        DWORD bytesReturned = 0;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &trimDescriptor, sizeof(trimDescriptor), &bytesReturned, nullptr))
        {
            information.m_profile += trimDescriptor.TrimEnabled ? "_SSD" : "_HDD";
            if (reportHardware)
            {

                AZ_Trace("Streamer",
                    "    Drive type: %s\n",
                    trimDescriptor.TrimEnabled ? "SSD" : "HDD");
            }
        }
        else
        {
            if (reportHardware)
            {
                AZ_Trace("Streamer", "    Drive type couldn't be determined.");
            }
        }
    }

    static void CollectAlignmentRequirements(HANDLE deviceHandle, DriveInformation& information, bool reportHardware)
    {
        STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR alignmentDescriptor{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageAccessAlignmentProperty;

        DWORD bytesReturned = 0;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &alignmentDescriptor, sizeof(alignmentDescriptor), &bytesReturned, nullptr))
        {
            information.m_physicalSectorSize = aznumeric_caster(alignmentDescriptor.BytesPerPhysicalSector);
            information.m_logicalSectorSize = aznumeric_caster(alignmentDescriptor.BytesPerLogicalSector);
            if (reportHardware)
            {
                AZ_Trace(
                    "Streamer",
                    "    Physical sector size: %i bytes\n"
                    "    Logical sector size: %i bytes\n",
                    alignmentDescriptor.BytesPerPhysicalSector,
                    alignmentDescriptor.BytesPerLogicalSector);
            }
        }
    }

    static void CollectDriveInfo(HANDLE deviceHandle, [[maybe_unused]] const char* driveName, bool reportHardware)
    {
        STORAGE_DEVICE_DESCRIPTOR sizeRequest{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageDeviceProperty;

        DWORD bytesReturned = 0;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &sizeRequest, sizeof(sizeRequest), &bytesReturned, nullptr))
        {
            auto header = reinterpret_cast<STORAGE_DESCRIPTOR_HEADER*>(&sizeRequest);
            auto buffer = AZStd::unique_ptr<char[]>(new char[header->Size]);

            if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
                buffer.get(), header->Size, &bytesReturned, nullptr))
            {
                if (reportHardware)
                {
                    [[maybe_unused]] auto deviceDescriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer.get());
                    AZ_Trace("Streamer",
                        "Drive info for '%s':\n"
                        "    Id: %s%s%s%s%s\n",
                        driveName,
                        deviceDescriptor->VendorIdOffset != 0 ? buffer.get() + deviceDescriptor->VendorIdOffset : "",
                        deviceDescriptor->VendorIdOffset != 0 ? " " : "",
                        deviceDescriptor->ProductIdOffset != 0 ? buffer.get() + deviceDescriptor->ProductIdOffset : "",
                        deviceDescriptor->ProductIdOffset != 0 ? " " : "",
                        deviceDescriptor->ProductRevisionOffset != 0 ? buffer.get() + deviceDescriptor->ProductRevisionOffset : "");
                }
            }
        }
    }

    static void CollectDriveIoCapability(HANDLE deviceHandle, DriveInformation& information, bool reportHardware)
    {
        STORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR capabilityDescriptor{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageDeviceIoCapabilityProperty;

        DWORD bytesReturned = 0;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &capabilityDescriptor, sizeof(capabilityDescriptor), &bytesReturned, nullptr))
        {
            information.m_ioChannelCount = aznumeric_caster(capabilityDescriptor.LunMaxIoCount);
            if (reportHardware)
            {
                AZ_Trace(
                    "Streamer",
                    "    Max IO count (LUN): %i\n"
                    "    Max IO count (Adapter): %i\n",
                    capabilityDescriptor.LunMaxIoCount,
                    capabilityDescriptor.AdapterMaxIoCount);
            }
        }
    }

    static void CollectDriveSeekPenalty(HANDLE deviceHandle, DriveInformation& information, bool reportHardware)
    {
        DWORD bytesReturned = 0;

        // Get seek penalty information.
        DEVICE_SEEK_PENALTY_DESCRIPTOR seekPenaltyDescriptor{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageDeviceSeekPenaltyProperty;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &seekPenaltyDescriptor, sizeof(seekPenaltyDescriptor), &bytesReturned, nullptr))
        {
            information.m_hasSeekPenalty = seekPenaltyDescriptor.IncursSeekPenalty ? true : false;
            if (reportHardware)
            {
                AZ_Trace("Streamer",
                    "    Has seek penalty: %s\n",
                    information.m_hasSeekPenalty ? "Yes" : "No");
            }
        }

    }

    static bool IsDriveUsed(AZ::IO::PathView driveId)
    {
        bool driveFound{};
        auto IsDriveInUse = [&driveId, &driveFound](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            AZ::IO::FixedMaxPath runtimePath;
            if (visitArgs.m_registry.Get(runtimePath.Native(), visitArgs.m_jsonKeyPath) && runtimePath.RootName() == driveId)
            {
                // Halt iteration if there exist O3DE is using a path from the drive
                driveFound = true;
                return AZ::SettingsRegistryInterface::VisitResponse::Done;
            }

            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };


        auto settingsRegistry = SettingsRegistry::Get();
        AZ::SettingsRegistryVisitorUtils::VisitObject(*settingsRegistry, IsDriveInUse, SettingsRegistryMergeUtils::FilePathsRootKey);

        return driveFound;
    }

    static bool CollectHardwareInfo(HardwareInformation& hardwareInfo, bool addAllDrives, bool reportHardware)
    {
        char drives[512];
        if (::GetLogicalDriveStringsA(sizeof(drives) - 1, drives))
        {
            AZStd::unordered_map<DWORD, DriveInformation> driveMappings;
            char* driveIt = drives;
            do
            {
                UINT driveType = ::GetDriveTypeA(driveIt);
                AZ::IO::PathView driveRootName = AZ::IO::PathView(driveIt).RootName();
                // Only a selective set of devices that share similar behavior are supported, in particular
                // drives that have magnetic or solid state storage. All types of buses (usb, sata, etc)
                // are supported except network drives. If network support is needed it's better to use the
                // virtual file system. Optical drives are not supported as they are currently not in use
                // on any platform for games, as games are expected to be downloaded or installed to storage.
                if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE || driveType == DRIVE_RAMDISK)
                {
                    if (!addAllDrives && !IsDriveUsed(driveRootName))
                    {
                        if (reportHardware)
                        {
                            AZ_Trace("Streamer", "Skipping drive '%s' because no paths make use of it.\n", driveIt);
                        }
                        while (*driveIt++);
                        continue;
                    }

                    AZStd::string deviceName = R"(\\.\)";
                    deviceName += driveRootName.Native();

                    HANDLE deviceHandle = ::CreateFileA(
                        deviceName.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
                    if (deviceHandle != INVALID_HANDLE_VALUE)
                    {
                        DWORD bytesReturned = 0;
                        // Get the device number so multiple partitions and mappings are handled by the same drive.
                        STORAGE_DEVICE_NUMBER storageDeviceNumber{};
                        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0,
                            &storageDeviceNumber, sizeof(storageDeviceNumber), &bytesReturned, nullptr))
                        {
                            auto driveInformationEntry = driveMappings.find(storageDeviceNumber.DeviceNumber);
                            if (driveInformationEntry == driveMappings.end())
                            {
                                DriveInformation driveInformation;
                                driveInformation.m_paths.emplace_back(driveIt);

                                CollectIoAdaptor(deviceHandle, driveInformation, driveIt, reportHardware);
                                if (driveInformation.m_supportsQueuing)
                                {
                                    CollectDriveInfo(deviceHandle, driveIt, reportHardware);
                                    AppendDriveTypeToProfile(deviceHandle, driveInformation, reportHardware);
                                    CollectDriveIoCapability(deviceHandle, driveInformation, reportHardware);
                                    CollectDriveSeekPenalty(deviceHandle, driveInformation, reportHardware);
                                    CollectAlignmentRequirements(deviceHandle, driveInformation, reportHardware);

                                    hardwareInfo.m_maxPhysicalSectorSize =
                                        AZStd::max(hardwareInfo.m_maxPhysicalSectorSize, driveInformation.m_physicalSectorSize);
                                    hardwareInfo.m_maxLogicalSectorSize =
                                        AZStd::max(hardwareInfo.m_maxLogicalSectorSize, driveInformation.m_logicalSectorSize);
                                    hardwareInfo.m_maxPageSize = AZStd::max(hardwareInfo.m_maxPageSize, driveInformation.m_pageSize);
                                    hardwareInfo.m_maxTransfer = AZStd::max(hardwareInfo.m_maxTransfer, driveInformation.m_maxTransfer);

                                    driveMappings.insert({ storageDeviceNumber.DeviceNumber, AZStd::move(driveInformation) });

                                    if (reportHardware)
                                    {
                                        AZ_Trace("Streamer", "\n");
                                    }
                                }
                                else
                                {
                                    if (reportHardware)
                                    {
                                        AZ_Trace(
                                            "Streamer", "Skipping drive '%s' because device does not support queuing requests.\n", driveIt);
                                    }
                                }
                            }
                            else
                            {
                                if (reportHardware)
                                {
                                    AZ_Trace(
                                        "Streamer", "Drive '%s' is on the same storage drive as '%s'.\n",
                                        driveIt, driveInformationEntry->second.m_paths[0].c_str());
                                }
                                driveInformationEntry->second.m_paths.emplace_back(driveIt);
                            }
                        }
                        else
                        {
                            if (reportHardware)
                            {
                                AZ_Trace(
                                    "Streamer", "Skipping drive '%s' because device is not registered with OS as a storage device.\n",
                                    driveIt);
                            }
                        }
                        ::CloseHandle(deviceHandle);
                    }
                    else
                    {
                        AZ_Warning("Streamer", false,
                            "Skipping drive '%s' because device information can't be retrieved.\n", driveIt);
                    }
                }
                else
                {
                    if (reportHardware)
                    {
                        AZ_Trace("Streamer", "Skipping drive '%s', as it the type of drive is not supported.\n", driveIt);
                    }
                }

                // Move to next drive string. GetLogicalDriveStrings fills the target buffer with null-terminated strings, for instance
                // "E:\\0F:\\0G:\\0\0". The code below reads forward till the next \0 and the following while will continue iterating until
                // the entire buffer has been used.
                while (*driveIt++);
            } while (*driveIt);

            DriveList driveList;
            driveList.reserve(driveMappings.size());
            for (auto& drive : driveMappings)
            {
                driveList.push_back(AZStd::move(drive.second));
            }
            hardwareInfo.m_profile = driveList.size() == 1 ? driveList.front().m_profile : "Generic";
            hardwareInfo.m_platformData = AZStd::make_any<DriveList>(AZStd::move(driveList));

            return !driveList.empty();
        }
        else
        {
            return false;
        }
    }

    bool CollectIoHardwareInformation(HardwareInformation& info, bool includeAllHardware, bool reportHardware)
    {
        if (!CollectHardwareInfo(info, includeAllHardware, reportHardware))
        {
            // The numbers below are based on common defaults from a local hardware survey.
            info.m_maxPageSize = 4096;
            info.m_maxTransfer = 512_kib;
            info.m_maxPhysicalSectorSize = 4096;
            info.m_maxLogicalSectorSize = 512;
            info.m_profile = "Generic";
        }
        return true;
    }

    void ReflectNative(ReflectContext* context)
    {
        WindowsStorageDriveConfig::Reflect(context);
    }
} // namespace AZ::IO
