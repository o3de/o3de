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
    QT_FORWARD_DECLARE_CLASS(O3DEObjectDownloadWorker)

    class O3DEObjectDownloadController : public QObject
    {
        Q_OBJECT

    public:
        explicit O3DEObjectDownloadController(QWidget* parent = nullptr);
        ~O3DEObjectDownloadController();

        void AddGemDownload(const QString& m_gemName);

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
            return m_gemNames[0];
        }
    public slots:
        void Start();
        void UpdateUIProgress(int progress);
        void HandleResults(const QString& result);
        void HandleCancel();

    signals:
        void StartGemDownload(const QString& gemName);
        void Done(bool success = true);
        void GemDownloadProgress(int percentage);

    private:
        O3DEObjectDownloadWorker* m_worker;
        QThread m_workerThread;
        QWidget* m_parent;
        AZStd::vector<QString> m_gemNames;

        int m_lastProgress;
    };
} // namespace O3DE::ProjectManager
