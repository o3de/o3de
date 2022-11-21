/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzFramework/ProjectManager/ProjectManager.h>

int main(int argc, char* argv[])
{
    const AZ::Debug::Trace tracer;
    // Verify a project path can be found, launch the project manager and shut down otherwise
    if (AzFramework::ProjectManager::CheckProjectPathProvided(argc, argv) == AzFramework::ProjectManager::ProjectPathCheckResult::ProjectManagerLaunched)
    {
        return 2;
    }
    using CryEditMain = int (*)(int, char*[]);
    constexpr const char CryEditMainName[] = "CryEditMain";

    AZ::AllocatorInstance<AZ::OSAllocator>::Create();

    auto handle = AZ::DynamicModuleHandle::Create("EditorLib");
    [[maybe_unused]] const bool loaded = handle->Load(true);
    AZ_Assert(loaded, "EditorLib could not be loaded");

    int ret = 1;
    if (auto fn = handle->GetFunction<CryEditMain>(CryEditMainName); fn != nullptr)
    {
        ret = AZStd::invoke(fn, argc, argv);
    }

    handle = {};
    AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    return ret;
}
