// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <${SanitizedCppName}ModuleInterface.h>
#include <Clients/${SanitizedCppName}SystemComponent.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}Module
        : public ${SanitizedCppName}ModuleInterface
    {
    public:
        AZ_RTTI(${SanitizedCppName}Module, ${SanitizedCppName}ModuleTypeId, ${SanitizedCppName}ModuleInterface);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}Module, AZ::SystemAllocator);
    };
} // namespace ${SanitizedCppName}

AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ${SanitizedCppName}::${SanitizedCppName}Module)
