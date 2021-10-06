/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProfilerModuleInterface.h>
#include <ProfilerSystemComponent.h>

namespace Profiler
{
    class ProfilerModule
        : public ProfilerModuleInterface
    {
    public:
        AZ_RTTI(ProfilerModule, "{1908f95f-30b9-4e27-8ae7-1e9fe487ef87}", ProfilerModuleInterface);
        AZ_CLASS_ALLOCATOR(ProfilerModule, AZ::SystemAllocator, 0);
    };
}// namespace Profiler

AZ_DECLARE_MODULE_CLASS(Gem_Profiler, Profiler::ProfilerModule)
