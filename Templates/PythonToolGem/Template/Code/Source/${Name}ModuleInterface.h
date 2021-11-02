// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}ModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(${SanitizedCppName}ModuleInterface, "{${Random_Uuid}}", AZ::Module);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}ModuleInterface, AZ::SystemAllocator, 0);

        ${SanitizedCppName}ModuleInterface()
        {
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
            };
        }
    };
}// namespace ${SanitizedCppName}
