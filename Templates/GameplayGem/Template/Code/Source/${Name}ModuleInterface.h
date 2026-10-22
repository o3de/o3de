// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

#include <AzCore/Module/Module.h>
#include <${Name}/${SanitizedCppName}TypeIds.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}ModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(${SanitizedCppName}ModuleInterface, ${SanitizedCppName}ModuleInterfaceTypeId, AZ::Module);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}ModuleInterface, AZ::SystemAllocator);

        ${SanitizedCppName}ModuleInterface();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
} // namespace ${SanitizedCppName}
