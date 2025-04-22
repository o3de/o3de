/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Preprocessor/Enum.h>

namespace AZ::RHI
{
    //! A list of popular vendor Ids. 
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(VendorId, uint32_t,
        (Unknown,       0),
        (Intel,         0x8086),
        (nVidia,        0x10de),
        (AMD,           0x1002),
        (Qualcomm,      0x5143),
        (Samsung,       0x1099),
        (ARM,           0x13B5),
        (Warp,          0x1414),
        (Apple,         0x106B) 
    );

    void ReflectVendorIdEnums(ReflectContext* context);

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
        VendorId m_vendorId = VendorId::Unknown;
        uint32_t m_deviceId = 0;
        uint32_t m_driverVersion = 0;
        AZStd::array<size_t, HeapMemoryLevelCount> m_heapSizePerLevel = {{}};
    };

    //! The GPU driver related information like unsupported versions, minimum version supported by the RHI.
    class PhysicalDeviceDriverInfo
    {
    public:
        AZ_RTTI(AZ::RHI::PhysicalDeviceDriverInfo, "{0063AFB9-C4F1-40F5-9F46-FEC631732F55}");
        static void Reflect(AZ::ReflectContext* context);
        virtual ~PhysicalDeviceDriverInfo() = default;

    private:
        struct Version
        {
            uint32_t m_encodedVersion;
            AZStd::string m_readableVersion;
        };

        VendorId m_vendorId;
        Version m_minVersion;
        AZStd::vector<Version> m_versionsWithIssues;

        void PrintRequiredDiverInfo() const;

        friend class PhysicalDeviceDriverValidator;
        friend class JsonPhysicalDeviceDriverInfoSerializer;
    };

    //! Validator for the current GPU driver.
    //! If the driver doesn't meet the requirements defined by RHI, a clear message will be output at RHI initialization time.
    class PhysicalDeviceDriverValidator
    {
    public:
        AZ_RTTI(AZ::RHI::PhysicalDeviceDriverValidator, "{EA11001D-5A5D-43D6-A90C-77E5E44273FC}");
        static void Reflect(AZ::ReflectContext* context);
        virtual ~PhysicalDeviceDriverValidator() = default;

        enum class ValidationResult
        {
            Supported,            //! The version meets the minimum requirement and no known issues.
            SupportedWithIssues,  //! The version meets the minimum requirement but with known issues.
            Unsupported,          //! The version doesn't meet the minimum requirement.
            MissingInfo           //! RHI doesn't define the requirements the drivers of a certain vendor.
        };

        ValidationResult ValidateDriverVersion(const PhysicalDeviceDescriptor& descriptor) const;

    private:
        AZStd::unordered_map<VendorId, PhysicalDeviceDriverInfo> m_driverInfo;
    };

    AZ_TYPE_INFO_SPECIALIZE(VendorId, "{12E63C56-976A-4575-B89F-1AE8C6D104D4}");
}
