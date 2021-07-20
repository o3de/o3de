/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <mutex>
#include <functional>

#include <QObject>
#include <QMap>
#endif

namespace News
{
    class QtDownloader;

    //! QtDownloadManager handles multiple asynchronous downloads
    class QtDownloadManager
        : public QObject
    {
        Q_OBJECT
    public:
        QtDownloadManager();
        ~QtDownloadManager();

        //! Asynchronously download a file from the input url and return it as QByteArray via the success callback
        /*!
        \param url - file url to download
        \param downloadSuccessCallback - if download is successful pass file's data as QByteArray
        \param downloadFailCallback - if download failed, pass error message
        */
        void Download(const QString& url,
            std::function<void(QByteArray)> downloadSuccessCallback,
            std::function<void()> downloadFailCallback);

        //! Aborts all currently active downloads. Success/failure callbacks will not be called.
        void Abort();

    private:
        void successfulReply(int downloadId, QByteArray data);
        void failedReply(int downloadId);

        QtDownloader* m_worker = nullptr;

        struct DownloadResponses
        {
            std::function<void(QByteArray)> downloadSuccessCallback;
            std::function<void()> downloadFailCallback;
        };
        QMap<int, DownloadResponses> m_downloads;
    };
}
