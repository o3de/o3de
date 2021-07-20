/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QtDownloadManager.h"
#include "QtDownloader.h"

namespace News
{

QtDownloadManager::QtDownloadManager()
    : QObject()
    , m_worker(new QtDownloader) // this will start the thread which does downloads
{
    // make sure the response handlers are queued connections as the worker runs in a different thread
    connect(m_worker, &QtDownloader::failed, this, &QtDownloadManager::failedReply, Qt::QueuedConnection);
    connect(m_worker, &QtDownloader::successfullyFinished, this, &QtDownloadManager::successfulReply, Qt::QueuedConnection);
}

QtDownloadManager::~QtDownloadManager()
{
    // NOTE: we don't delete the QtDownloader; it deletes itself.
    // We just tell it to stop
    m_worker->Finish();
}

void QtDownloadManager::Download(const QString& url,
    std::function<void(QByteArray)> downloadSuccessCallback,
    std::function<void()> downloadFailCallback)
{
    int downloadId = m_worker->Download(url);
    m_downloads[downloadId] = { downloadSuccessCallback, downloadFailCallback };
}

void QtDownloadManager::Abort()
{
    m_worker->Abort();
    m_downloads.clear();
}

void QtDownloadManager::successfulReply(int downloadId, QByteArray data)
{
    m_downloads[downloadId].downloadSuccessCallback(data);
    m_downloads.remove(downloadId);
}

void QtDownloadManager::failedReply(int downloadId)
{
    m_downloads[downloadId].downloadFailCallback();
    m_downloads.remove(downloadId);
}

} // namespace News

#include "NewsShared/ResourceManagement/moc_QtDownloadManager.cpp"
