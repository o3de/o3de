/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/SentryCrashReportingSystemComponent.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace CrashReporting
{
    class SentryCrashReportingModule : public AZ::Module
    {
    public:
        AZ_RTTI(SentryCrashReportingModule, "{3C8F0E5A-7D64-4A1B-B0E9-9F5C2A44D8E1}", AZ::Module);
        AZ_CLASS_ALLOCATOR(SentryCrashReportingModule, AZ::SystemAllocator);

        SentryCrashReportingModule()
        {
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    SentryCrashReportingSystemComponent::CreateDescriptor(),
                });
        }

        //! Returned components are created automatically on the system entity, which is what makes
        //! merely enabling this gem enough to turn crash reporting on.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SentryCrashReportingSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), CrashReporting::SentryCrashReportingModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_CrashReporting, CrashReporting::SentryCrashReportingModule)
#endif
