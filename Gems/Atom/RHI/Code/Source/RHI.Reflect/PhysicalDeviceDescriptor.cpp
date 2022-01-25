/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/PhysicalDeviceDescriptor.h>
#include <Atom/RHI.Reflect/PhysicalDeviceDriverInfoSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <sstream>

namespace AZ
{
    namespace RHI
    {
        AZ_ENUM_DEFINE_REFLECT_UTILITIES(VendorId);

        void ReflectVendorIdEnums(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                VendorIdReflect(*serializeContext);
            }
        }

        void PhysicalDeviceDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PhysicalDeviceDescriptor>()
                    ->Version(1)
                    ->Field("m_description", &PhysicalDeviceDescriptor::m_description)
                    ->Field("m_type", &PhysicalDeviceDescriptor::m_type)
                    ->Field("m_vendorId", &PhysicalDeviceDescriptor::m_vendorId)
                    ->Field("m_deviceId", &PhysicalDeviceDescriptor::m_deviceId)
                    ->Field("m_driverVersion", &PhysicalDeviceDescriptor::m_driverVersion)
                    ->Field("m_heapSizePerLevel", &PhysicalDeviceDescriptor::m_heapSizePerLevel);
            }
        }

        void PhysicalDeviceDriverInfo::Reflect(AZ::ReflectContext* context)
        {
            if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonPhysicalDeviceDriverInfoSerializer>()->HandlesType<PhysicalDeviceDriverInfo>();
            }
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PhysicalDeviceDriverInfo>()
                    ->Version(1);
            }
        }

        void PhysicalDeviceDriverInfo::PrintRequiredDiverInfo() const
        {
            std::stringstream ss;
            ss << "Vendor " << ToString(m_vendorId).data() << " must have a minimum version of " << m_minVersion.m_readableVersion.c_str();
            auto badVersionIter = m_versionsWithIssues.begin();
            if (badVersionIter != m_versionsWithIssues.end())
            {
                ss << ".\nAnd the following versions are known to have issues with Atom: " << badVersionIter->m_readableVersion.c_str();
                ++badVersionIter;
            }
            while (badVersionIter != m_versionsWithIssues.end())
            {
                ss << ", " << badVersionIter->m_readableVersion.c_str();
                ++badVersionIter;
            }
            ss << ".\n";
            AZ_Printf("RHISystem", ss.str().c_str());
        }

        void PhysicalDeviceDriverValidator::Reflect(AZ::ReflectContext* context)
        {
            PhysicalDeviceDriverInfo::Reflect(context);

            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PhysicalDeviceDriverValidator>()->Version(1)->Field(
                    "driverInfo", &PhysicalDeviceDriverValidator::m_driverInfo);
            }
        }

        PhysicalDeviceDriverValidator::ValidationResult PhysicalDeviceDriverValidator::ValidateDriverVersion(const PhysicalDeviceDescriptor& descriptor) const
        {
            // [GFX TODO] Add driver info for other platforms besides Windows. Currently, avoid spamming warnings.
            // ATOM-14967 [RHI][Metal] - Address driver version validator for Mac
            if (m_driverInfo.size() == 0)
            {
                return ValidationResult::MissingInfo;
            }

            auto iter = m_driverInfo.find(descriptor.m_vendorId);

            if (iter == m_driverInfo.end())
            {
                AZ_Warning(
                    "PhysicalDeviceDriverValidator", false,
                    "Unable to verify driver versions. Vendor %s infomation is not provided in PhysicalDeviceDriverInfo.setreg.",
                    ToString(descriptor.m_vendorId).data());
                return ValidationResult::MissingInfo;
            }

            const PhysicalDeviceDriverInfo& driverInfo = iter->second;

            if (descriptor.m_driverVersion < driverInfo.m_minVersion.m_encodedVersion)
            {
                AZ_Warning(
                    "PhysicalDeviceDriverValidator", false, "Vendor %s should use a driver version higher than or equal to %s.",
                    ToString(descriptor.m_vendorId).data(), driverInfo.m_minVersion.m_readableVersion.c_str());
                driverInfo.PrintRequiredDiverInfo();
                return ValidationResult::Unsupported;
            }
            else
            {
                for (const auto& badVersion : driverInfo.m_versionsWithIssues)
                {
                    if (badVersion.m_encodedVersion == descriptor.m_driverVersion)
                    {
                        AZ_Warning(
                            "PhysicalDeviceDriverValidator", false, "Vendor %s driver version %s is known to have some issues with Atom.",
                            ToString(descriptor.m_vendorId).data(), badVersion.m_readableVersion.c_str());
                        driverInfo.PrintRequiredDiverInfo();
                        return ValidationResult::SupportedWithIssues;
                    }
                }
            }

            return ValidationResult::Supported;
        }
    }
}
