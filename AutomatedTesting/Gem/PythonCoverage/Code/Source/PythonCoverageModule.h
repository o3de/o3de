
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <PythonCoverageSystemComponent.h>

#pragma once

#pragma optimize("", off)

namespace PythonCoverage
{
    class PythonCoverageModule : public AZ::Module
    {
    public:
        AZ_RTTI(PythonCoverageModule, "{dc706de0-22c4-4b05-9b99-438692afc082}", AZ::Module);
        AZ_CLASS_ALLOCATOR(PythonCoverageModule, AZ::SystemAllocator, 0);

        PythonCoverageModule();

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
} // namespace PythonCoverage
