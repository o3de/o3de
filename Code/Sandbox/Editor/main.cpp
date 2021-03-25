/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzFramework/ProjectManager/ProjectManager.h>

int main(int argc, char* argv[])
{
    // Verify a project path can be found, launch the project manager and shut down otherwise
    if (AzFramework::ProjectManager::CheckProjectPathProvided(argc, argv) == AzFramework::ProjectManager::ProjectPathCheckResult::ProjectManagerLaunched)
    {
        return 2;
    }
    using CryEditMain = int (*)(int, char*[]);
    constexpr const char CryEditMainName[] = "CryEditMain";

    AZ::Environment::Attach(AZ::Environment::GetInstance());
    AZ::AllocatorInstance<AZ::OSAllocator>::Create();

    auto handle = AZ::DynamicModuleHandle::Create("EditorLib");
    [[maybe_unused]] const bool loaded = handle->Load(true);
    AZ_Assert(loaded, "EditorLib could not be loaded");

    if (auto fn = handle->GetFunction<CryEditMain>(CryEditMainName); fn != nullptr)
    {
        const int ret = AZStd::invoke(fn, argc, argv);

        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
        AZ::Environment::Detach();

        return ret;
    }

    AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    AZ::Environment::Detach();
    return 1;
}
