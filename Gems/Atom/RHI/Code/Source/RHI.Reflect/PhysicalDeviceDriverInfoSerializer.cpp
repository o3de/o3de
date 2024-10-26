/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/PhysicalDeviceDriverInfoSerializer.h>

namespace AZ::RHI
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonPhysicalDeviceDriverInfoSerializer, SystemAllocator);

    namespace
    {
        static constexpr const char FieldVendor[]             = "vendor";
        static constexpr const char FieldMinVersion[]         = "minVersion";
        static constexpr const char FieldVersionsWithIssues[] = "versionsWithIssues";
    }

    uint32_t ConvertVersionNumber(VendorId vendor, const AZStd::string& versionStr)
    {
        AZStd::vector<uint32_t> versions;
        constexpr const char del = '.';
        size_t start = 0;
        size_t end = versionStr.find(del);
        while (end != AZStd::string::npos)
        {
            versions.push_back(aznumeric_cast<uint32_t>(atoi(versionStr.substr(start, end - start).c_str())));
            start = end + 1;
            end = versionStr.find(del, start);
        }
        versions.push_back(aznumeric_cast<uint32_t>(atoi(versionStr.substr(start, end).c_str())));

        uint32_t version = 0;

        // Version encoding using Vulkan format
        switch (vendor)
        {
        case VendorId::nVidia:
            // nVidia version format xx.xx.1x.xxxx
            // e.g. 27.21.14.5687
            if (versions.size() == 4)
            {
                version = (((versions[2] % 10) * 100 + versions[3] / 100) << 22) | ((versions[3] % 100) << 14);
            }
            // nVidia version format xxx.xx
            // e.g 456.87
            else if (versions.size() == 2)
            {
                version = (versions[0] << 22) | (versions[1] << 14);
            }
            else
            {
                AZ_Warning(
                    "PhysicalDeviceDriverInfoSerializer", false, "Vendor %s version %s is using an unknown format.",
                    ToString(vendor).data(), versionStr.c_str());
            }
            break;
        case VendorId::Intel:
            // Intel version format xx.xx.1xx.xxxx
            // e.g. 25.20.100.6793
            if (versions.size() == 4)
            {
                version = (versions[2] << 14) | versions[3];
            }
            // Intel version format 1xx.xxxx
            // e.g. 100.6793
            else if (versions.size() == 2)
            {
                version = (versions[0] << 14) | versions[1];
            }
            else
            {
                AZ_Warning(
                    "PhysicalDeviceDriverInfoSerializer", false, "Vendor %s version %s is using an unknown format.",
                    ToString(vendor).data(), versionStr.c_str());
            }
            break;
        default:
            // Default using Vulkan's standard
            // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#extendingvulkan-coreversions-versionnumbers
            if (versions.size() == 3)
            {
                version = (versions[0] << 22) | (versions[1] << 12) | versions[2];
            }
            else
            {
                AZ_Warning(
                    "PhysicalDeviceDriverInfoSerializer", false, "Vendor %s version %s is using an unknown format.",
                    ToString(vendor).data(), versionStr.c_str());
            }
        }

        return version;
    }

    JsonSerializationResult::Result JsonPhysicalDeviceDriverInfoSerializer::Load(
        void* outputValue, [[maybe_unused]] const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<PhysicalDeviceDriverInfo>() == outputValueTypeId,
            "Unable to deserialize PhysicalDeviceDriverInfo to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());

        PhysicalDeviceDriverInfo* driverInfo = reinterpret_cast<PhysicalDeviceDriverInfo*>(outputValue);

        JsonSerializationResult::ResultCode result(JsonSerializationResult::Tasks::ReadField);

        driverInfo->m_vendorId = VendorId::Unknown;
        AZStd::vector<AZStd::string> badVersions;
        result.Combine(ContinueLoadingFromJsonObjectField(&driverInfo->m_vendorId, azrtti_typeid(driverInfo->m_vendorId), inputValue, FieldVendor, context));
        result.Combine(ContinueLoadingFromJsonObjectField(&driverInfo->m_minVersion.m_readableVersion, azrtti_typeid(driverInfo->m_minVersion.m_readableVersion), inputValue, FieldMinVersion, context));
        result.Combine(ContinueLoadingFromJsonObjectField(&badVersions, azrtti_typeid(badVersions), inputValue, FieldVersionsWithIssues, context));

        driverInfo->m_minVersion.m_encodedVersion = ConvertVersionNumber(driverInfo->m_vendorId, driverInfo->m_minVersion.m_readableVersion);
        driverInfo->m_versionsWithIssues.reserve(driverInfo->m_versionsWithIssues.size());
        for (const AZStd::string& badVersion : badVersions)
        {
            driverInfo->m_versionsWithIssues.push_back(
                PhysicalDeviceDriverInfo::Version{ConvertVersionNumber(driverInfo->m_vendorId, badVersion), badVersion}
            );
        }

        if (result.GetProcessing() == JsonSerializationResult::Processing::Completed)
        {
            return context.Report(result, "Successfully loaded physical device driver Info.");
        }
        else
        {
            return context.Report(result, "Partially loaded physical device driver Info.");
        }
    }

    JsonSerializationResult::Result JsonPhysicalDeviceDriverInfoSerializer::Store(
        rapidjson::Value& outputValue, const void* inputValue, [[maybe_unused]] const void* defaultValue,
        [[maybe_unused]] const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<PhysicalDeviceDriverInfo>() == valueTypeId,
            "Unable to serialize PhysicalDeviceDriverInfo to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());

        namespace JSR = JsonSerializationResult;
        JsonSerializationResult::ResultCode result(JSR::Tasks::WriteValue);

        const PhysicalDeviceDriverInfo* driverInfo = reinterpret_cast<const PhysicalDeviceDriverInfo*>(inputValue);

        AZStd::vector<AZStd::string> badVersions;
        badVersions.reserve(driverInfo->m_versionsWithIssues.size());
        for (const auto& badVersion : driverInfo->m_versionsWithIssues)
        {
            badVersions.push_back(badVersion.m_readableVersion);
        }

        VendorId defaultVendorId = VendorId::Unknown;
        result.Combine(ContinueStoringToJsonObjectField(outputValue, FieldVendor, &driverInfo->m_vendorId, &defaultVendorId, azrtti_typeid(driverInfo->m_vendorId), context));
        result.Combine(ContinueStoringToJsonObjectField(outputValue, FieldMinVersion, &driverInfo->m_minVersion.m_readableVersion, nullptr, azrtti_typeid(driverInfo->m_minVersion.m_readableVersion), context));
        result.Combine(ContinueStoringToJsonObjectField(outputValue, FieldVersionsWithIssues, &badVersions, nullptr, azrtti_typeid(badVersions), context));

        if (result.GetProcessing() == JsonSerializationResult::Processing::Completed)
        {
            return context.Report(result, "Successfully stored physical device driver Info.");
        }
        else
        {
            return context.Report(result, "Partially stored physical device driver Info.");
        }
    }
}
