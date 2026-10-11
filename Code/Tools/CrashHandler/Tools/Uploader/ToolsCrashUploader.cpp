/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QUrl>

#include <fstream>
#include <memory>

#include "UI/ui_submit_report.h"

namespace O3de
{
    static const char* settingKey_issueReportLink = "/O3DE/Settings/Links/Issue/Create";
    static const char* issueReportLinkFallback = "https://github.com/o3de/o3de/issues/new/choose";

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
    #if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
        char noConfirmation[64]{};
        size_t variableSize = 0;
        getenv_s(&variableSize, noConfirmation, AZ_ARRAY_SIZE(noConfirmation), "LY_NO_CONFIRM");
        if (variableSize == 0)
    #else
        const char* noConfirmation = getenv("LY_NO_CONFIRM");
        if (noConfirmation == nullptr)
    #endif
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

            QAtomicInt dialogResult{ -1 };

            const bool manualReport = m_submissionToken.compare("manual") == 0;
            ::CrashUploader::SendReportDialog confirmDialog{ manualReport, nullptr };
            confirmDialog.SetApplicationName(m_executableName.c_str());

            confirmDialog.setWindowIcon(QIcon(":/Icons/lyeditor.ico"));

            QFileInfo fileInfo = QFileInfo(reportPath);
            confirmDialog.SetReportDirectory(fileInfo.absolutePath());

            QString summaryText;
            summaryText += "Report ID:  " + QString::fromStdString(report.uuid.ToString()) + "\n";
            summaryText += "Created:    " + QDateTime::fromSecsSinceEpoch(static_cast<qint64>(report.creation_time)).toString(Qt::ISODate) + "\n";
            summaryText += "Executable: " + QString::fromStdString(m_executableName) + "\n";
            summaryText += "Dump file:  " + fileInfo.absoluteFilePath() + "\n";
            confirmDialog.SetSummaryText(summaryText);

            confirmDialog.setWindowFlags((confirmDialog.windowFlags() | Qt::WindowStaysOnTopHint) & ~Qt::WindowContextHelpButtonHint);

            confirmDialog.exec();

            while (dialogResult == -1)
            {
                app.processEvents();
                dialogResult = confirmDialog.result();
            }

            const bool accepted = dialogResult == QDialog::Accepted;

            if (accepted)
            {
                const QString userComments = confirmDialog.GetUserComments();
                const bool wantsContact = confirmDialog.WantsContact();
                if (!userComments.isEmpty() || wantsContact)
                {
                    QString commentsFilePath = fileInfo.absolutePath() + "/user_comments.txt";
                    std::ofstream commentsFile(commentsFilePath.toStdString());
                    commentsFile << "Contact consent: " << (wantsContact ? "yes" : "no") << "\n\n";
                    commentsFile << userComments.toStdString();
                    commentsFile.close();

#if defined(AZ_PLATFORM_WINDOWS)
                    m_uploadPaths.push_back(base::FilePath(commentsFilePath.toStdWString()));
#else
                    m_uploadPaths.push_back(base::FilePath(commentsFilePath.toStdString()));
#endif
                }

                if (confirmDialog.WantsRestart() && !m_executableName.empty())
                {
                    // Best-effort relaunch: the original command line (project, level, args) isn't
                    // recorded by the crash handler, so this only restarts the bare executable.
                    QString exePath = QCoreApplication::applicationDirPath() + "/" + QString::fromStdString(m_executableName);
                    if (!QProcess::startDetached(exePath, {}))
                    {
                        LOG(ERROR) << "Failed to restart " << m_executableName << " after crash report submission";
                    }
                }
            }

            if (manualReport && accepted)
            {
                QDesktopServices::openUrl(QUrl(fileInfo.absolutePath()));

                AZStd::string reportIssueUrl;
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    settingsRegistry->Get(reportIssueUrl, settingKey_issueReportLink);
                }

                if (reportIssueUrl.empty())
                {
                    reportIssueUrl = issueReportLinkFallback;
                }
                QDesktopServices::openUrl(QUrl(reportIssueUrl.data()));
            }
            return accepted;
        }
#endif
        return true;
    }
}
