/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QString>
#include <QThread>
#include <AzCore/std/containers/vector.h>
#endif

QT_FORWARD_DECLARE_CLASS(QProcess)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(DownloadWorker)

    class DownloadController : public QObject
    {
        Q_OBJECT

    public:
        explicit DownloadController(QWidget* parent = nullptr);
        ~DownloadController();

        void AddGemDownload(const QString& gemName);
        void CancelGemDownload(const QString& gemName);

        bool IsDownloadQueueEmpty()
        {
            return m_gemNames.empty();
        }

        const AZStd::vector<QString>& GetDownloadQueue() const
        {
            return m_gemNames;
        }

        const QString& GetCurrentDownloadingGem() const
        {
            if (!m_gemNames.empty())
            {
                return m_gemNames[0];
            }
            else
            {
                static const QString emptyString;
                return emptyString;
            }
        }
    public slots:
        void UpdateUIProgress(int bytesDownloaded, int totalBytes);
        void HandleResults(const QString& result, const QString& detailedError);

    signals:
        void StartGemDownload(const QString& gemName);
        void Done(const QString& gemName, bool success = true);
        void GemDownloadAdded(const QString& gemName);
        void GemDownloadRemoved(const QString& gemName);
        void GemDownloadProgress(const QString& gemName, int bytesDownloaded, int totalBytes);

    private:
        DownloadWorker* m_worker;
        QThread m_workerThread;
        QWidget* m_parent;
        AZStd::vector<QString> m_gemNames;
    };
} // namespace O3DE::ProjectManager
