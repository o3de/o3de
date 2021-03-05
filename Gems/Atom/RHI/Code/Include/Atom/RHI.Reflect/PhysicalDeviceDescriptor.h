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

#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * A list of popular vendor Ids. This is not an enum at the moment because it's an incomplete list.
         */
        namespace VendorId
        {
            const uint32_t Unknown = 0;
            const uint32_t Intel = 0x8086;
            const uint32_t nVidia = 0x10de;
            const uint32_t AMD = 0x1002;
            const uint32_t Qualcomm = 0x5143;
            const uint32_t Samsung = 0x1099;
            const uint32_t ARM = 0x13B5;
        }

        enum class PhysicalDeviceType : uint32_t
        {
            Unknown = 0,

            /// An integrated GPU sharing system memory with the CPU.
            GpuIntegrated,

            /// A discrete GPU separated from the CPU by a bus. The GPU has its own separate memory heap.
            GpuDiscrete,

            /// A GPU abstracted through a virtual machine.
            GpuVirtual,

            /// A CPU software rasterizer.
            Cpu,

            /// A fake device for mocking or stubbing.
            Fake,

            Count
        };

        const uint32_t PhysicalDeviceTypeCount = static_cast<uint32_t>(PhysicalDeviceType::Count);

        class PhysicalDeviceDescriptor
        {
        public:
            AZ_RTTI(PhysicalDeviceDescriptor, "{22052601-3C81-4FD2-AD46-1AE00F01E95E}");
            static void Reflect(AZ::ReflectContext* context);
            virtual ~PhysicalDeviceDescriptor() = default;

            AZStd::string m_description;
            PhysicalDeviceType m_type = PhysicalDeviceType::Unknown;
            uint32_t m_vendorId = VendorId::Unknown;
            uint32_t m_deviceId = 0;
            uint32_t m_driverVersion = 0;
            AZStd::array<size_t, HeapMemoryLevelCount> m_heapSizePerLevel = {{}};
        };
    }
}
