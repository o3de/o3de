/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RemoteToolsModuleInterface.h>
#include <RemoteToolsSystemComponent.h>

namespace RemoteTools
{
    class RemoteToolsModule
        : public RemoteToolsModuleInterface
    {
    public:
        AZ_RTTI(RemoteToolsModule, "{86ed333f-1f40-497f-ac31-9de31dee9371}", RemoteToolsModuleInterface);
        AZ_CLASS_ALLOCATOR(RemoteToolsModule, AZ::SystemAllocator);
    };
}// namespace RemoteTools

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), RemoteTools::RemoteToolsModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_RemoteTools, RemoteTools::RemoteToolsModule)
#endif
