/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
