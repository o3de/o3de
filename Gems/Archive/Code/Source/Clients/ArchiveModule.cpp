/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Archive/ArchiveTypeIds.h>
#include <ArchiveModuleInterface.h>
#include "ArchiveSystemComponent.h"

namespace Archive
{
    class ArchiveModule
        : public ArchiveModuleInterface
    {
    public:
        AZ_RTTI(ArchiveModule, ArchiveModuleTypeId, ArchiveModuleInterface);
        AZ_CLASS_ALLOCATOR(ArchiveModule, AZ::SystemAllocator);
    };
}// namespace Archive

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Archive::ArchiveModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Archive, Archive::ArchiveModule)
#endif
