/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Outcome/Outcome.h>

#include <QObject>
#include <QProcessEnvironment>
#include <QString>
#endif

QT_FORWARD_DECLARE_CLASS(QProcess)

namespace O3DE::ProjectManager
{
    class O3DEObjectDownloadWorker : public QObject
    {
        // QProcess::waitForFinished uses -1 to indicate that the process should not timeout
        static constexpr int MaxBuildTimeMSecs = -1;
        // Download was cancelled
        inline static const QString DownloadCancelled = QObject::tr("Download Cancelled.");

        Q_OBJECT

    public:
        explicit O3DEObjectDownloadWorker();
        ~O3DEObjectDownloadWorker() = default;

    public slots:
        void StartDownload();
        void SetGemToDownload(const QString& gemName, bool downloadNow = true);

    signals:
        void UpdateProgress(int progress);
        void Done(QString result = "");

    private:

        QProcess* m_configProjectProcess = nullptr;
        QProcess* m_buildProjectProcess = nullptr;

        QString m_gemName;
        int m_downloadProgress;
    };
} // namespace O3DE::ProjectManager
