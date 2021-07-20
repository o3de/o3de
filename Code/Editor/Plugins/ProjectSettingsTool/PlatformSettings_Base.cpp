/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProjectSettingsTool_precompiled.h"
#include "PlatformSettings_Base.h"

#include "PlatformSettings_common.h"
#include "Validators.h"

namespace ProjectSettingsTool
{
    void BaseSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BaseSettings>()
                ->Version(2)
                ->Field("project_name", &BaseSettings::m_projectName)
                ->Field("product_name", &BaseSettings::m_productName)
                ->Field("executable_name", &BaseSettings::m_executableName)
                ->Field("project_path", &BaseSettings::m_projectPath)
                ->Field("project_output_folder", &BaseSettings::m_projectOutputFolder)
                ->Field("code_folder", &BaseSettings::m_codeFolder)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<BaseSettings>("Project Settings", "All core settings for the game project and package and deployment.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(Handlers::LinkedLineEdit, &BaseSettings::m_projectName, "Project Name", "The name of the project.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::FileName))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::ProjectName)
                        ->Attribute(Attributes::LinkedProperty, Identfiers::IosBundleName)
                    ->DataElement(Handlers::LinkedLineEdit, &BaseSettings::m_productName, "Product Name", "The project's user facing name.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::IsNotEmpty))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::ProductName)
                        ->Attribute(Attributes::LinkedProperty, Identfiers::IosDisplayName)
                    ->DataElement(Handlers::LinkedLineEdit, &BaseSettings::m_executableName, "Executable Name", "The project launcher's name.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::FileName))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::ExecutableName)
                        ->Attribute(Attributes::LinkedProperty, Identfiers::IosExecutableName)
                    ->DataElement(Handlers::QValidatedLineEdit, &BaseSettings::m_projectPath, "Project Path", "The project root folder path .")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::FileNameOrEmpty))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::ProductName)
                        ->Attribute(Attributes::LinkedProperty, Identfiers::ExecutableName)
                    ->DataElement(Handlers::QValidatedLineEdit, &BaseSettings::m_projectOutputFolder, "Output Folder", "The folder the packed project will be exported to.")
                    ->DataElement(Handlers::QValidatedLineEdit, &BaseSettings::m_codeFolder, "Code Folder (legacy)", "A legacy setting specifing the folder for this project's code.")
                ;
            }
        }
    }
} // namespace ProjectSettingsTool
