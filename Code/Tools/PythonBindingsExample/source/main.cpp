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

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <source/Application.h>

namespace PythonBindingsExample
{
    //! This function returns the build system target name
    AZStd::string_view GetBuildTargetName()
    {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view{ LY_CMAKE_TARGET };
    }
}

int main(int argc, char* argv[])
{
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    int runSuccess = 0;
    {
        PythonBindingsExample::Application application(&argc, &argv);
        // The AZ::ComponentApplication constructor creates the settings registry
        // so the CMake target name can be added at this point
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), PythonBindingsExample::GetBuildTargetName());
        application.SetUp();
        runSuccess = application.Run() ? 0 : 1;
    }
    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    return runSuccess;
}
