/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RecastNavigationModuleInterface.h>

namespace RecastNavigation
{
    class RecastNavigationModule
        : public RecastNavigationModuleInterface
    {
    public:
        AZ_RTTI(RecastNavigationModule, "{8178c5eb-97fa-4fe6-a0b6-b7678ed205e4}", RecastNavigationModuleInterface);
        AZ_CLASS_ALLOCATOR(RecastNavigationModule, AZ::SystemAllocator, 0);
    };
}// namespace RecastNavigation

AZ_DECLARE_MODULE_CLASS(Gem_RecastNavigation, RecastNavigation::RecastNavigationModule)
