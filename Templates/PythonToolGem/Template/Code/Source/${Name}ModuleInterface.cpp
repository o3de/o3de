// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${Name}ModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <${Name}/${Name}TypeIds.h>

namespace ${SanitizedCppName}
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(${SanitizedCppName}ModuleInterface,
        "${SanitizedCppName}ModuleInterface", ${SanitizedCppName}ModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(${SanitizedCppName}ModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(${SanitizedCppName}ModuleInterface, AZ::SystemAllocator);

    ${SanitizedCppName}ModuleInterface::${SanitizedCppName}ModuleInterface()
    {
    }

    AZ::ComponentTypeList ${SanitizedCppName}ModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
        };
    }
} // namespace ${SanitizedCppName}
