/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <RecastNavigationModuleInterface.h>
#include <RecastNavigationSystemComponent.h>

namespace RecastNavigation
{
    class RecastNavigationModule
        : public RecastNavigationModuleInterface
    {
    public:
        AZ_RTTI(RecastNavigationModule, "{a8fb0082-78ab-4ca6-8f63-68c98f1a6a6d}", RecastNavigationModuleInterface);
        AZ_CLASS_ALLOCATOR(RecastNavigationModule, AZ::SystemAllocator);
    };
}// namespace RecastNavigation

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), RecastNavigation::RecastNavigationModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_RecastNavigation, RecastNavigation::RecastNavigationModule)
#endif
