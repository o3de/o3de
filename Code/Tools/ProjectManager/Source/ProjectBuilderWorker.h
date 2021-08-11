/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ProjectInfo.h>
#include <AzCore/Outcome/Outcome.h>

#include <QObject>
#endif

QT_FORWARD_DECLARE_CLASS(QProcess)

namespace O3DE::ProjectManager
{
    class ProjectBuilderWorker : public QObject
    {
        // QProcess::waitForFinished uses -1 to indicate that the process should not timeout
        static constexpr int MaxBuildTimeMSecs = -1;
        // Build was cancelled
        inline static const QString BuildCancelled = QObject::tr("Build Cancelled.");

        Q_OBJECT

    public:
        explicit ProjectBuilderWorker(const ProjectInfo& projectInfo);
        ~ProjectBuilderWorker() = default;

        QString GetLogFilePath() const;

    public slots:
        void BuildProject();

    signals:
        void UpdateProgress(int progress);
        void Done(QString result = "");

    private:
        AZ::Outcome<void, QString> BuildProjectForPlatform();
        void QStringToAZTracePrint(const QString& error);

        QProcess* m_configProjectProcess = nullptr;
        QProcess* m_buildProjectProcess = nullptr;
        ProjectInfo m_projectInfo;

        int m_progressEstimate;
    };
} // namespace O3DE::ProjectManager
