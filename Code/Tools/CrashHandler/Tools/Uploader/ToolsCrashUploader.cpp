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

// LY Editor Crashpad Upload Handler Extension

#include <ToolsCrashUploader.h>
#include <SendReportDialog.h>

#include <CrashSupport.h>

#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/typetraits/underlying_type.h>

#include <QString>
#include <QApplication>

#include <memory>

#include "UI/ui_submit_report.h"

namespace O3de
{
    void InstallCrashUploader(int& argc, char* argv[])
    {
        O3de::CrashUploader::SetCrashUploader(std::make_shared<O3de::ToolsCrashUploader>(argc, argv));
    }
    QString GetReportString(const std::wstring& reportPath)
    {
        return  QString::fromStdWString(reportPath);
    }
    QString GetReportString(const std::string& reportPath)
    {
        return QString::fromStdString( reportPath );
    }

    ToolsCrashUploader::ToolsCrashUploader(int& argCount, char** argv) :
        CrashUploader(argCount, argv)
    {

    }

    std::string ToolsCrashUploader::GetRootFolder()
    {
        std::string returnPath;
        ::CrashHandler::GetExecutableFolder(returnPath);

        std::string devRoot{ "/dev/" };

        auto devpos = returnPath.rfind(devRoot);
        if (devpos != std::string::npos)
        {
            /* Cut off everything beyond \\dev\\ */
            returnPath.erase(devpos + devRoot.length());
        }
        return returnPath;
    }

    bool ToolsCrashUploader::CheckConfirmation(const crashpad::CrashReportDatabase::Report& report)
    {
        if (m_noConfirmation)
        {
            return true;
        }
#if !AZ_TRAIT_OS_PLATFORM_APPLE
        AZ_PUSH_DISABLE_WARNING(4996, "-Wunknown-warning-option")
        const char* noConfirmation = getenv("LY_NO_CONFIRM");
        AZ_POP_DISABLE_WARNING
        if (noConfirmation == nullptr)
        {
            int argCount = 0;

            AzQtComponents::StyleManager styleManager{ nullptr };
            QApplication app{ argCount, nullptr };
            AZ::IO::FixedMaxPath engineRootPath;
            {
                AZ::ComponentApplication componentApplication;
                auto settingsRegistry = AZ::SettingsRegistry::Get();
                settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            }
            styleManager.initialize(&app, engineRootPath);

            QString reportPath{ GetReportString(report.file_path.value()) };
 
            QString reportString{ GetReportString(report.file_path.value()).toStdString().c_str() };
            QAtomicInt dialogResult{ -1 };

            ::CrashUploader::SendReportDialog confirmDialog{ nullptr };
            confirmDialog.SetApplicationName(m_executableName.c_str());

            confirmDialog.setWindowIcon(QIcon(":/Icons/editor_icon.ico"));
            confirmDialog.SetReportText(reportString.toStdString().c_str());
            confirmDialog.setWindowFlags((confirmDialog.windowFlags() | Qt::WindowStaysOnTopHint) & ~Qt::WindowContextHelpButtonHint);

            confirmDialog.exec();

            while (dialogResult == -1)
            {
                app.processEvents();
                dialogResult = confirmDialog.result();
            }
            return dialogResult == QDialog::Accepted;
        }
#endif
        return true;
    }
}
