/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PlatformSettings_Base.h"

#include "PlatformSettings_common.h"
#include "Validators.h"

#include <AzFramework/Translation/TranslationDef.h>

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
                editContext->Class<BaseSettings>(
                    QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Project Settings"),
                    QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "All core settings for the game project and package and deployment."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(Handlers::LinkedLineEdit, &BaseSettings::m_projectName,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Project Name"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "The name of the project."))
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::FileName))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::ProjectName)
                    ->DataElement(Handlers::LinkedLineEdit, &BaseSettings::m_productName,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Product Name"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "The project's user facing name."))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::ProductName)
                    ->DataElement(Handlers::LinkedLineEdit, &BaseSettings::m_executableName,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Executable Name"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "The project launcher's name."))
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::FileNameOrEmpty))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::ExecutableName)
                    ->DataElement(Handlers::QValidatedLineEdit, &BaseSettings::m_projectPath,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Project Path"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "The project root folder path."))
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::FileNameOrEmpty))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::ProductName)
                    ->DataElement(Handlers::QValidatedLineEdit, &BaseSettings::m_projectOutputFolder,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Output Folder"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "The folder the packed project will be exported to."))
                    ->DataElement(Handlers::QValidatedLineEdit, &BaseSettings::m_codeFolder,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Code Folder (legacy)"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "A legacy setting specifying the folder for this project's code."))
                ;
            }
        }
    }
} // namespace ProjectSettingsTool
