/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Application.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>

#include <AzToolsFramework/Thumbnails/ThumbnailerNullComponent.h>
#include <SliceConverterEditorEntityContextComponent.h>

namespace AZ::SerializeContextTools
{
    // SerializeContextTools is a full ToolsApplication that will load a project's Gem DLLs and initialize the system components.
    // This level of initialization is required to get all the serialization contexts and asset handlers registered, so that when
    // data transformations take place, none of the data is dropped due to not being recognized.
    // However, as a simplification, anything requiring Python or Qt is skipped during initialization:
    // - The gem_autoload.serializecontexttools.setreg file disables autoload for QtForPython, EditorPythonBindings, and PythonAssetBuilder
    // - The system component initialization below uses ThumbnailerNullComponent so that other components relying on a ThumbnailService
    //   can still be started up, but the thumbnail service itself won't do anything.  The real ThumbnailerComponent uses Qt, which is why
    //   it isn't used.

    Application::Application(int argc, char** argv, AZ::IO::FileDescriptorCapturer* stdoutCapturer)
        : Application(argc, argv, stdoutCapturer, {})
    {
    }

    Application::Application(int argc, char** argv, ComponentApplicationSettings componentAppSettings)
        : Application(argc, argv, nullptr, AZStd::move(componentAppSettings))
    {
    }
    Application::Application(int argc, char** argv, AZ::IO::FileDescriptorCapturer* stdoutCapturer, ComponentApplicationSettings componentAppSettings)
        : AzToolsFramework::ToolsApplication(&argc, &argv, AZStd::move(componentAppSettings))
        , m_stdoutCapturer(stdoutCapturer)
    {
        // We need a specialized variant of EditorEntityContextComponent for the SliceConverter, so we register the descriptor here.
        RegisterComponentDescriptor(AzToolsFramework::SliceConverterEditorEntityContextComponent::CreateDescriptor());

        AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
        if (projectPath.empty())
        {
            AZ_TracePrintf("Serialize Context Tools", "Unable to determine the project path . "
                "Make sure project has been set or provide via the -project-path option on the command line. (See -help for more info.)");
            return;
        }

        size_t configSwitchCount = m_commandLine.GetNumSwitchValues("config");
        if (configSwitchCount > 0)
        {
            AZ::IO::FixedMaxPath absConfigFilePath = projectPath / m_commandLine.GetSwitchValue("config", configSwitchCount - 1);
            if (AZ::IO::SystemFile::Exists(absConfigFilePath.c_str()))
            {
                m_configFilePath = AZStd::move(absConfigFilePath);
            }
        }

        // Merge the build system generated setting registry file by using either "Editor" or
        // and "${ProjectName}_GameLauncher" as a specialization
        auto projectName = AZ::Utils::GetProjectName();
        if (projectName.empty())
        {
            AZ_Error("Serialize Context Tools", false, "Unable to query the project name from settings registry");
        }
        else
        {
            AZ::SettingsRegistryInterface::Specializations projectSpecializations{ projectName };

            // If a project specialization has been passed in via the command line, use it.
            if (size_t specializationCount = m_commandLine.GetNumSwitchValues("specializations"); specializationCount > 0)
            {
                for (size_t specializationIndex = 0; specializationIndex < specializationCount; ++specializationIndex)
                {
                    projectSpecializations.Append(m_commandLine.GetSwitchValue("specializations", specializationIndex));
                }
            }
            else
            {
                // Otherwise, if a config file was passed in, auto-set the specialization based on the config file name.
                AZ::IO::PathView configFilenameStem = m_configFilePath.Stem();
                if (AZ::StringFunc::Equal(configFilenameStem.Native(), "Editor"))
                {
                    projectSpecializations.Append("editor");
                }
                else if (AZ::StringFunc::Equal(configFilenameStem.Native(), "Game"))
                {
                    projectSpecializations.Append(projectName + "_GameLauncher");
                }

                // If the "dumptypes" or "createtype" supplied attempt to load the editor gem dependencies
                if (m_commandLine.GetNumMiscValues() > 0 &&
                    (m_commandLine.GetMiscValue(0) == "dumptypes" || m_commandLine.GetMiscValue(0) == "createtype"
                        || m_commandLine.GetMiscValue(0) == "dumpsc"))
                {
                    projectSpecializations.Append("editor");
                }
            }

            // Used the project specializations to merge the gem modules dependencies *.setreg files
            auto registry = AZ::SettingsRegistry::Get();
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_TargetBuildDependencyRegistry(*registry,
                AZ_TRAIT_OS_PLATFORM_CODENAME, projectSpecializations);
        }
    }

    const char* Application::GetConfigFilePath() const
    {
        return m_configFilePath.c_str();
    }

    void Application::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
    {
        appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Tool;
    }

    void Application::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
    {
        AZ::ComponentApplication::SetSettingsRegistrySpecializations(specializations);
        specializations.Append("serializecontexttools");
    }

    AZ::ComponentTypeList Application::GetRequiredSystemComponents() const
    {
        // By default, we use all of the standard system components.
        AZ::ComponentTypeList components = AzToolsFramework::ToolsApplication::GetRequiredSystemComponents();

        // Also add in the ThumbnailerNullComponent so that components requiring a ThumbnailService can still be started up.
        components.emplace_back(azrtti_typeid<AzToolsFramework::Thumbnailer::ThumbnailerNullComponent>());

        // The Slice Converter requires a specialized variant of the EditorEntityContextComponent that exposes the ability
        // to disable the behavior of activating entities on creation.  During conversion, the creation flow will be triggered,
        // but entity activation requires a significant amount of subsystem initialization that's unneeded for conversion.
        // So, to get around this, we swap out EditorEntityContextComponent with SliceConverterEditorEntityContextComponent.
        components.erase(
            AZStd::remove(
                components.begin(), components.end(), azrtti_typeid<AzToolsFramework::EditorEntityContextComponent>()),
            components.end());
        components.emplace_back(azrtti_typeid<AzToolsFramework::SliceConverterEditorEntityContextComponent>());
        return components;
    }

    void Application::SetStdoutCapturer(AZ::IO::FileDescriptorCapturer* stdoutCapturer)
    {
        m_stdoutCapturer = stdoutCapturer;
    }
    AZ::IO::FileDescriptorCapturer* Application::GetStdoutCapturer() const
    {
        return m_stdoutCapturer;
    }
} // namespace AZ::SerializeContextTools
