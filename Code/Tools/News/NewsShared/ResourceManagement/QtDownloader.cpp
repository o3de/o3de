/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QtDownloader.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QThread>


namespace News
{
    QtDownloader::QtDownloader()
        : m_thread(new QThread())
    {
        // make sure that everything that QObject::connects to us knows we're running in a different thread
        moveToThread(m_thread);

        // handle clean up of both ourselves and of our thread.
        // We manage thread clean up so that it can keep running and be cleaned up later, regardless of what
        // the thing that created the QtDownloader does
        connect(m_thread, &QThread::finished, m_thread, [this] {
            m_thread->deleteLater();
            deleteLater();
            });

        auto abortDownloadsHandler = [this] {
            auto replies = m_downloads.keys();
            for (QNetworkReply* reply : replies)
            {
                reply->abort();
            }

            m_downloads.clear();
        };

        auto queueDownloadHandler = [this](int downloadId, QString url) {
            if (m_networkManager)
            {
                QNetworkReply* reply = m_networkManager->get(QNetworkRequest(QUrl(url)));
                m_downloads.insert(reply, downloadId);
            }
        };

        auto createNetworkManagerHandler = [this] {
            m_networkManager = new QNetworkAccessManager;
            connect(m_networkManager, &QNetworkAccessManager::finished, this, &QtDownloader::downloadFinished);
        };

        auto deleteNetworkManagerHandler = [this] {
            delete m_networkManager;
            m_networkManager = nullptr;
        };

        auto quitHandler = [this] {
            // call quit via this callback, so that it executes in the running thread.
            // QThread::quit() is actually blocking and waits until everything finishes, so
            // we don't want to call it in the main thread
            m_thread->quit();
        };

        // make sure the response handlers are queued connections as the worker runs in one thread, but these triggers
        // will be emitted from the main thread
        connect(this, &QtDownloader::triggerAbortAll, this, abortDownloadsHandler, Qt::QueuedConnection);
        connect(this, &QtDownloader::triggerDownload, this, queueDownloadHandler, Qt::QueuedConnection);
        connect(this, &QtDownloader::triggerQuit, this, quitHandler, Qt::QueuedConnection);

        // create/delete the QNetworkAccessManager in our thread, to ensure that any slowdowns caused by
        // having to create network connectors / load drivers are done in our non-ui thread.
        // make sure that these connections are direct so that network requests can't predate the network engine itself
        connect(m_thread, &QThread::started, this, createNetworkManagerHandler, Qt::DirectConnection);
        connect(m_thread, &QThread::finished, this, deleteNetworkManagerHandler, Qt::DirectConnection);

        m_thread->start();
    }

    QtDownloader::~QtDownloader()
    {
    }

    int QtDownloader::Download(const QString& url)
    {
        // create a unique id for this download
        int downloadId = m_lastId++;

        // trigger a download running in our worker thread
        Q_EMIT triggerDownload(downloadId, url);

        return downloadId;
    }

    void QtDownloader::Abort()
    {
        // trigger an abort in our worker thread
        Q_EMIT triggerAbortAll();
    }

    void QtDownloader::Finish()
    {
        // trigger a quit in our worker thread
        Q_EMIT triggerQuit();
    }

    void QtDownloader::downloadFinished(QNetworkReply* reply)
    {
        // Note: this will run in our worker thread
        int downloadId = m_downloads[reply];

        // emit the signal back to the main thread indicating that we're finished, either
        // successfully or unsuccessfully
        if (reply->error() == QNetworkReply::NoError)
        {
            Q_EMIT successfullyFinished(downloadId, reply->readAll());
        }
        else
        {
            Q_EMIT failed(downloadId);
        }

        // clean up the reply; have to do this later, according to the Qt docs
        reply->deleteLater();

        // make sure to remove our reference to this reply from our list of active downloads
        m_downloads.remove(reply);
    }

    #include "NewsShared/ResourceManagement/moc_QtDownloader.cpp"
}
