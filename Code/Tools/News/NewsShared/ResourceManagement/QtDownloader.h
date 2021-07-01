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

class QNetworkAccessManager;
class QNetworkReply;
class QThread;

namespace News
{
    //! QtDownloader is a wrapper around Qt's file download functions
    /*!
        The QtDownloader spins up another thread and does all downloads in that thread.
        The public slot methods (Finish, Download and Abort) can all be called from any thread.
        The response signals (successfullyFinished and failed) should be QObject::connect to with
        Qt::QueuedConnection, as they will be emitted from the worker thread.
    */
    class QtDownloader
        : public QObject
    {
        Q_OBJECT

    public:
        QtDownloader();
        ~QtDownloader();

    public Q_SLOTS:
        void Finish();

        int Download(const QString& url);
        void Abort();

    Q_SIGNALS:
        void successfullyFinished(int downloadId, QByteArray data);
        void failed(int downloadId);

        // ***********************************************
        // private - DO NOT CONNECT TO outside of the class!
        // (qt signals can't be made private)
        void triggerAbortAll();
        void triggerDownload(int downloadId, QString url);
        void triggerQuit();
        // ***********************************************

    private:
        void downloadFinished(QNetworkReply* reply);

        int m_lastId = 0;

        QMap<QNetworkReply*, int> m_downloads;
        QNetworkAccessManager* m_networkManager = nullptr;
        QThread* m_thread;
    };
}
