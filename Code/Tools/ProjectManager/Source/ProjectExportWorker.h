/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ProjectInfo.h>
#include <AzCore/Outcome/Outcome.h>

#include <QObject>
#include <QProcessEnvironment>
#endif

QT_FORWARD_DECLARE_CLASS(QProcess)

namespace O3DE::ProjectManager
{
    class ProjectExportWorker : public QObject
    {
        // QProcess::waitForFinished uses -1 to indicate that the process should not timeout
        static constexpr int MaxExportTimeMSecs = -1;
        inline static const QString ExportCancelled = QObject::tr("Export Cancelled.");
        inline static const QString LogOpenFailureMsg = QObject::tr("Failed to open log file.");
        inline static const QString LogPathFailureMsg = QObject::tr("Failed to retrieve log file path.");
        Q_OBJECT

    public:
        explicit ProjectExportWorker(const ProjectInfo& projectInfo);
        ~ProjectExportWorker() = default;

        AZ::Outcome<QString, QString> GetLogFilePath() const;

    public slots:
        void ExportProject();

    signals:
        void UpdateProgress(QString lastLine);
        void Done(QString result = "");

    private:
        AZ::Outcome<void, QString> ExportProjectForPlatform();
        void QStringToAZTracePrint(const QString& error);
        AZ::Outcome<QStringList, QString> ConstructKillProcessCommandArguments(const QString& pidToKill) const;
        QString GetO3DECLIString() const;
        QProcess* m_exportProjectProcess = nullptr;
        ProjectInfo m_projectInfo;
        QString exportScript;
    };
} // namespace O3DE::ProjectManager
