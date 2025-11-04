/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    const AZ::Debug::Trace tracer;
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
    return runSuccess;
}
