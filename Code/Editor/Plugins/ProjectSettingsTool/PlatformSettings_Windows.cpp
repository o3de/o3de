/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PlatformSettings_Windows.h"

#include "PlatformSettings_common.h"
#include "Validators.h"

#include <AzCore/IO/FileIO.h>

namespace ProjectSettingsTool
{
    void WindowsGraphics::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<WindowsGraphics>()
                ->Version(1)
                ->Field("graphicsAPI", &WindowsGraphics::m_graphicsAPI)
                ->Field("validationMode", &WindowsGraphics::m_validationMode);

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<WindowsGraphics>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &WindowsGraphics::m_graphicsAPI,
                        "Graphics API", "Select the primary graphics API")
                    ->Attribute(
                        AZ::Edit::Attributes::StringList,
                        []()
                        {
                            return AZStd::vector<AZStd::string>{ "DX12", "Vulkan" };
                        })
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &WindowsGraphics::m_validationMode,
                        "Validation Layers", "Set the validation mode for the RHI.")
                    ->Attribute(
                        AZ::Edit::Attributes::EnumValues,
                        AZStd::vector<AZ::Edit::EnumConstant<ValidationMode>>{
                            AZ::Edit::EnumConstant<ValidationMode>(
                                ValidationMode::Disabled, "Disabled"),
                            AZ::Edit::EnumConstant<ValidationMode>(
                                ValidationMode::Enabled, "Enabled  (Shows warnings and error messages)"),
                            AZ::Edit::EnumConstant<ValidationMode>(
                                ValidationMode::Verbose, "Verbose  (Shows warnings, errors, and informational messages)"),
                            AZ::Edit::EnumConstant<ValidationMode>(
                                ValidationMode::GPU, "GPU"),
                        });
            }
        }
    }

    const char* WindowsGraphics::ValidationModeToString(ValidationMode mode)
    {
        switch (mode)
        {
        case ValidationMode::Disabled:
            return "disable";
        case ValidationMode::Enabled:
            return "enable";
        case ValidationMode::Verbose:
            return "verbose";
        case ValidationMode::GPU:
            return "gpu";
        default:
            return "disable";
        }
    }

    void WindowsGraphics::SaveRHISettings(const AZ::IO::Path& settingsPath) const
    {
        AZStd::string jsonContent = AZStd::string::format(
            "{\n"
            "    \"O3DE\": {\n"
            "        \"Atom\": {\n"
            "            \"RHI\": {\n"
            "                \"FactoryManager\": {\n"
            "                    \"factoriesPriority\": [\n"
            "                        \"%s\",\n"
            "                        \"%s\"\n"
            "                    ]\n"
            "                }\n"
            "            },\n"
            "            \"rhi-device-validation\": \"%s\"\n"
            "        }\n"
            "    }\n"
            "}",
            m_graphicsAPI.c_str(),
            (m_graphicsAPI == "Vulkan") ? "DX12" : "Vulkan",
            ValidationModeToString(m_validationMode));

        AZ::IO::FileIOStream fileStream(settingsPath.c_str(), AZ::IO::OpenMode::ModeWrite);
        if (fileStream.IsOpen())
        {
            fileStream.Write(jsonContent.size(), jsonContent.c_str());
            fileStream.Close();
        }
    }

    void WindowsSettings::Reflect(AZ::ReflectContext* context)
    {
        WindowsGraphics::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<WindowsSettings>()
                ->Version(1)
                ->Field("graphics", &WindowsSettings::m_graphics);

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<WindowsSettings>("Windows Settings", "Configure settings for Windows platform")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &WindowsSettings::m_graphics,
                        "Graphics Settings",
                        "Configure Graphics settings for Windows platform");
            }
        }
    }
} // namespace ProjectSettingsTool
