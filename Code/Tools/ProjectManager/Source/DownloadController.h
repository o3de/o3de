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
#include <AzCore/std/containers/deque.h>
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
            QString m_destinationPath;
            DownloadObjectType m_objectType;
        };

        explicit DownloadController(QWidget* parent = nullptr);
        ~DownloadController();

        void AddObjectDownload(const QString& objectName, const QString& destinationPath, DownloadObjectType objectType);
        void CancelObjectDownload(const QString& objectName, DownloadObjectType objectType);
        bool IsDownloadingObject(const QString& objectName, DownloadObjectType objectType);

        bool IsDownloadQueueEmpty()
        {
            return m_objects.empty();
        }

        const AZStd::deque<DownloadableObject>& GetDownloadQueue() const
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
        void StartObjectDownload(const QString& objectName, const QString& destinationPath, DownloadObjectType objectType, bool downloadNow);
        void Done(const QString& objectName, bool success = true);
        void ObjectDownloadAdded(const QString& objectName, DownloadObjectType objectType);
        void ObjectDownloadRemoved(const QString& objectName, DownloadObjectType objectType);
        void ObjectDownloadProgress(const QString& objectName, DownloadObjectType objectType, int bytesDownloaded, int totalBytes);

    private:
        DownloadWorker* m_worker;
        QThread m_workerThread;
        AZStd::deque<DownloadableObject> m_objects;
    };

} // namespace O3DE::ProjectManager
