// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <${Name}/${Name}TypeIds.h>
#include <${Name}ModuleInterface.h>
#include "${Name}SystemComponent.h"

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}Module
        : public ${SanitizedCppName}ModuleInterface
    {
    public:
        AZ_RTTI(${SanitizedCppName}Module, ${SanitizedCppName}ModuleTypeId, ${SanitizedCppName}ModuleInterface);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}Module, AZ::SystemAllocator);
    };
}// namespace ${SanitizedCppName}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ${SanitizedCppName}::${SanitizedCppName}Module)
#else
AZ_DECLARE_MODULE_CLASS(Gem_${SanitizedCppName}, ${SanitizedCppName}::${SanitizedCppName}Module)
#endif
