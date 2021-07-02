/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ResourceManifest.h"
#include "NewsShared/ResourceManagement/QtDownloadManager.h"
#include "NewsShared/ResourceManagement/Resource.h"
#include "NewsShared/ResourceManagement/ArticleDescriptor.h"

#include <QJsonArray>
#include <QByteArray>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

namespace News
{

    const QString ResourceManifest::MANIFEST_NAME = "resourceManifest";
    bool ResourceManifest::s_syncing = false;

    ResourceManifest::ResourceManifest(
        std::function<void()> syncSuccessCallback,
        std::function<void(ErrorCode)> syncFailCallback,
        std::function<void(QString, LogType)> syncUpdateCallback)
        : m_downloader(new QtDownloadManager)
        , m_syncSuccessCallback(syncSuccessCallback)
        , m_syncFailCallback(syncFailCallback)
        , m_syncUpdateCallback(syncUpdateCallback)
    {
    }

    ResourceManifest::~ResourceManifest()
    {
        // clean everything up
        DeleteResources();

        delete m_downloader;
    }

    Resource* ResourceManifest::FindById(const QString& id) const
    {
        return FindById(id, m_resources);
    }

    Resource* ResourceManifest::FindById(const QString& id, const QList<Resource*>& resources)
    {
        auto it = std::find_if(
            resources.begin(),
            resources.end(),
            [id](Resource* resource) -> bool
            {
                return resource->GetId().compare(id) == 0;
            });
        if (it == resources.end())
        {
            return nullptr;
        }
        return *it;
    }

    Resource* ResourceManifest::FindById(const QString& id,
        const QStack<Resource*>& resources)
    {
        auto it = std::find_if(
            resources.begin(),
            resources.end(),
            [id](Resource* resource) -> bool
            {
                return resource->GetId().compare(id) == 0;
            });
        if (it == resources.end())
        {
            return nullptr;
        }
        return *it;
    }

    void ResourceManifest::Sync()
    {
        if (s_syncing)
        {
            FailSync(ErrorCode::AlreadySyncing);
            return;
        }
        s_syncing = true;
        m_failed = false;

        m_syncUpdateCallback("Starting sync", LogInfo);

        ReadConfig();

        // first download the manifest json
        m_syncUpdateCallback("Downloading manifest", LogInfo);
        m_downloader->Download(QString(m_url).append(MANIFEST_NAME),
            std::bind(&ResourceManifest::OnDownloadSuccess, this, std::placeholders::_1),
            std::bind(&ResourceManifest::OnDownloadFail, this));
    }

    void ResourceManifest::Abort()
    {
        m_aborted = true;
        m_downloader->Abort();
    }

    void ResourceManifest::Reset()
    {
        if (s_syncing)
        {
            m_syncUpdateCallback("Sync is already running", LogError);
            return;
        }

        m_aborted = false;
        m_failed = false;
        m_version = -1;

        DeleteResources();

        m_order.clear();
    }

    QList<Resource*>::const_iterator ResourceManifest::begin() const
    {
        return m_resources.constBegin();
    }

    QList<Resource*>::const_iterator ResourceManifest::end() const
    {
        return m_resources.constEnd();
    }

    QList<QString> ResourceManifest::GetOrder() const
    {
        return m_order;
    }

    void ResourceManifest::OnDownloadSuccess(QByteArray data)
    {
        QJsonDocument doc(QJsonDocument::fromJson(data));
        if (doc.isNull())
        {
            FailSync(ErrorCode::FailedToParseManifest);
            return;
        }

        ErrorCode error = Read(doc.object());
        if (error != ErrorCode::None)
        {
            FailSync(error);
            return;
        }

        // check how many resources to sync
        PrepareForSync();
        // if there is anything to sync, do that
        if (m_syncLeft > 0)
        {
            m_syncUpdateCallback("Syncing resources", LogInfo);
            SyncResources();
        }
        // otherwise just finish sync
        else
        {
            m_syncUpdateCallback("No new resources to sync", LogInfo);
            FinishSync();
        }
    }

    void ResourceManifest::OnDownloadFail()
    {
        FailSync(ErrorCode::ManifestDownloadFail);
    }

    ErrorCode ResourceManifest::Read(const QJsonObject& json)
    {
        m_version = json["version"].toInt();
        QJsonArray resourceArray = json["resources"].toArray();
        // initially mark ALL existing resource for deletion
        QList<Resource*> toDelete = m_resources;

        for (auto resourceDoc : resourceArray)
        {
            auto pNewResource = new Resource(resourceDoc.toObject());
            // find local resource with the same id as new resource
            auto pOldResource = FindById(pNewResource->GetId(), m_resources);
            // if resource with the same id already exists then check its version
            if (pOldResource)
            {
                // local resource is outdated, keep it in delete list, and download new one instead
                if (pNewResource->GetVersion() > pOldResource->GetVersion())
                {
                    m_toDownload.push(pNewResource);
                }
                // local resource is newer or same version, keep it (remove from toDelete list)
                // and don't need to download new one
                else
                {
                    delete pNewResource;
                    toDelete.removeAll(pOldResource);
                }
            }
            // resource with same id not found
            else
            {
                m_toDownload.push(pNewResource);
            }
        }

        // delete everything that's not in s3
        for (auto pResource : toDelete)
        {
            RemoveResource(pResource);
            delete pResource;
        }

        // parse order of articles
        m_order.clear();
        QJsonArray orderArray = json["order"].toArray();
        for (auto idObject : orderArray)
        {
            m_order.append(idObject.toString());
        }
        return ErrorCode::None;
    }

    void ResourceManifest::PrepareForSync()
    {
        if (m_aborted)
        {
            m_syncLeft = 0;
        }

        m_syncLeft = m_toDownload.count();
    }

    void ResourceManifest::SyncResources()
    {
        DownloadResources();
    }

    void ResourceManifest::DownloadResources()
    {
        while (m_toDownload.count() > 0)
        {
            m_syncUpdateCallback(
                QString("Downloading: %1 resources left").arg(m_toDownload.count()),
                LogInfo);

            auto pResource = m_toDownload.pop();

            m_downloader->Download(QString(m_url).append(pResource->GetId()),
                //download success
                [&, pResource](QByteArray data)
                {
                    pResource->SetData(data);
                    AppendResource(pResource);
                    UpdateSync();
                },
                //download fail
                    [&, pResource]()
                {
                    m_failed = true;
                    delete pResource;
                    m_syncUpdateCallback("Failed to download resource", LogError);
                    UpdateSync();
                });
        }
    }

    void ResourceManifest::ReadConfig()
    {
        QFile file(QCoreApplication::applicationDirPath() + "/newsConfig.txt");
        if (file.exists())
        {
            if (file.open(QIODevice::ReadOnly))
            {
                QTextStream in(&file);
                m_url = in.readAll().trimmed();
                file.close();
            }
        }
    }

    void ResourceManifest::DeleteResources()
    {
        for (auto pResource : m_toDownload)
        {
            delete pResource;
        }
        m_toDownload.clear();

        for (auto pResource : m_resources)
        {
            delete pResource;
        }
        m_resources.clear();
    }

    void ResourceManifest::UpdateSync()
    {
        m_syncLeft--;
        if (m_syncLeft == 0)
        {
            if (!m_failed)
            {
                FinishSync();
            }
            else
            {
                FailSync(ErrorCode::FailedToSync);
            }
        }
    }

    void ResourceManifest::FinishSync()
    {
        if (!m_failed)
        {
            m_syncSuccessCallback();
        }
        else
        {
            m_syncFailCallback(m_errorCode);
        }
        s_syncing = false;
    }

    void ResourceManifest::FailSync(ErrorCode error)
    {
        m_failed = true;
        m_errorCode = error;
        FinishSync();
    }

    void ResourceManifest::AppendResource(Resource* pResource)
    {
        m_resources.append(pResource);
    }

    void ResourceManifest::RemoveResource(Resource* pResource)
    {
        m_resources.removeAll(pResource);
    }

}
