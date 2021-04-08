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

#pragma once

#include <AzCore/IO/Streamer/StreamerConfiguration.h>

namespace AZ::IO
{
    class WindowsStorageDriveConfig final :
        public IStreamerStackConfig
    {
    public:
        AZ_RTTI(AZ::IO::WindowsStorageDriveConfig, "{E9D8256B-EE40-4F60-8B1D-D88200CD1F10}", IStreamerStackConfig);
        AZ_CLASS_ALLOCATOR(WindowsStorageDriveConfig, SystemAllocator, 0);

        ~WindowsStorageDriveConfig() override = default;
        AZStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
            const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent) override;
        static void Reflect(ReflectContext* context);

    private:
        AZ::u32 m_maxFileHandles{ 32 };
        AZ::u32 m_maxMetaDataCache{ 32 };
        AZ::u32 m_overcommit{ 8 };
        bool m_enableFileSharing{ false };
        bool m_enableUnbufferedReads{ true };
        bool m_minimalReporting{ false };
    };
} // namespace AZ::IO
