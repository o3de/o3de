/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Document/MaterialDocumentModule.h>
#include <Atom/Viewport/MaterialViewportModule.h>
#include <Atom/Window/MaterialEditorWindowModule.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <MaterialEditorApplication.h>
#include <MaterialEditor_Traits_Platform.h>

namespace MaterialEditor
{
    //! This function returns the build system target name of "MaterialEditor"
    AZStd::string MaterialEditorApplication::GetBuildTargetName() const
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string{ LY_CMAKE_TARGET };
    }

    const char* MaterialEditorApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseMaterialEditor";
#elif defined(_DEBUG)
        return "DebugMaterialEditor";
#else
        return "ProfileMaterialEditor";
#endif
    }

    MaterialEditorApplication::MaterialEditorApplication(int* argc, char*** argv)
        : Base(argc, argv)
    {
        QApplication::setApplicationName("O3DE Material Editor");

        // The settings registry has been created at this point, so add the CMake target
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), GetBuildTargetName());
    }

    void MaterialEditorApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Base::CreateStaticModules(outModules);
        outModules.push_back(aznew MaterialDocumentModule);
        outModules.push_back(aznew MaterialViewportModule);
        outModules.push_back(aznew MaterialEditorWindowModule);
    }

    AZStd::vector<AZStd::string> MaterialEditorApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialEditor/" });
    }
} // namespace MaterialEditor
