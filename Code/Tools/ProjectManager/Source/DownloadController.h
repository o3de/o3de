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
        enum DownloadObjectType
        {
            Gem = 1 << 0,
            Project = 1 << 1,
            Template = 1 << 2
        };

        struct DownloadableObject
        {
            QString m_objectName;
            DownloadObjectType m_objectType;
        };

        explicit DownloadController(QWidget* parent = nullptr);
        ~DownloadController();

        void AddGemDownload(const QString& gemName);
        void CancelGemDownload(const QString& gemName);

        void AddProjectDownload(const QString& projectName);
        void CancelProjectDownload(const QString& projectName);

        bool IsDownloadQueueEmpty()
        {
            return m_objects.empty();
        }

        const AZStd::vector<DownloadableObject>& GetDownloadQueue() const
        {
            return m_objects;
        }

        const QString& GetCurrentDownloadingGem() const
        {
            if (!m_objects.empty())
            {
                return m_objects[0].m_objectName;
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
        void StartGemDownload(const QString& gemName, bool downloadNow);
        void StartProjectDownload(const QString& projectName, bool downloadNow);
        void StartTemplateDownload(const QString& templateName, bool downloadNow);
        void Done(const QString& gemName, bool success = true);
        void GemDownloadAdded(const QString& gemName);
        void GemDownloadRemoved(const QString& gemName);
        void GemDownloadProgress(const QString& gemName, int bytesDownloaded, int totalBytes);
        void ProjectDownloadAdded(const QString& projectName);
        void ProjectDownloadRemoved(const QString& projectName);
        void ProjectDownloadProgress(const QString& projectName, int bytesDownloaded, int totalBytes);

    private:
        DownloadWorker* m_worker;
        QThread m_workerThread;
        QWidget* m_parent;
        AZStd::vector<DownloadableObject> m_objects;
    };
} // namespace O3DE::ProjectManager
