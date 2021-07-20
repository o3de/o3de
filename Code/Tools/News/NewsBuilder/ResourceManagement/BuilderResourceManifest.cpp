/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BuilderResourceManifest.h"
#include "NewsShared/ResourceManagement/QtDownloadManager.h"
#include "NewsBuilder/S3Connector.h"
#include "UidGenerator.h"
#include "NewsShared/ResourceManagement/Resource.h"
#include "NewsBuilder/ResourceManagement/UploadDescriptor.h"
#include "NewsBuilder/ResourceManagement/DeleteDescriptor.h"
#include "NewsShared/ResourceManagement/ArticleDescriptor.h"
#include "NewsBuilder/ResourceManagement/ImageDescriptor.h"
#include "EndpointManager.h"

#include <QJsonArray>
#include <QByteArray>

#include <aws/core/utils/memory/stl/AwsStringStream.h>
#include <QMessageBox>

namespace News {

    BuilderResourceManifest::BuilderResourceManifest(
        std::function<void()> syncSuccessCallback,
        std::function<void(ErrorCode)> syncFailCallback,
        std::function<void(QString, LogType)> syncUpdateCallback)
        : ResourceManifest(syncSuccessCallback, syncFailCallback, syncUpdateCallback)
        , m_s3Connector(new S3Connector)
        , m_uidGenerator(new UidGenerator)
        , m_endpointManager(new EndpointManager) {}


    Resource* BuilderResourceManifest::AddArticle()
    {
        auto pResource = new Resource(
            QString("%1").arg(m_uidGenerator->GenerateUid()),
            "article");
        QJsonObject json;
        json["title"] = "New Article";
        json["body"] = "Enter article body here";
        json["imageId"] = "0";
        QJsonDocument doc(json);
        QByteArray data = doc.toJson(QJsonDocument::Compact).toStdString().data();
        pResource->SetData(data);
        m_toUpload.push(pResource);
        AppendResource(pResource);
        m_order.append(pResource->GetId());
        return pResource;
    }

    Resource* BuilderResourceManifest::AddImage(const QString& filename)
    {
        auto pResource = new Resource(
            QString("%1").arg(m_uidGenerator->GenerateUid()),
            "image");
        ImageDescriptor descriptor(*pResource);
        QString error;
        if (!descriptor.Read(filename, error))
        {
            m_syncUpdateCallback(error, LogError);
            delete pResource;
            return nullptr;
        }
        m_toUpload.push(pResource);
        AppendResource(pResource);
        return pResource;
    }

    void BuilderResourceManifest::UpdateResource(Resource* pResource)
    {
        if (!m_toUpload.contains(pResource))
        {
            pResource->SetVersion(pResource->GetVersion() + 1);
            m_toUpload.push(pResource);
        }
    }

    void BuilderResourceManifest::UseResource(const QString& id)
    {
        auto pResource = FindById(id, m_resources);
        if (!pResource)
        {
            return;
        }
        pResource->SetRefCount(pResource->GetRefCount() + 1);
        UpdateResource(pResource);
    }

    void BuilderResourceManifest::FreeResource(const QString& id)
    {
        auto pResource = FindById(id, m_resources);
        if (!pResource)
        {
            return;
        }
        if (m_toDelete.contains(pResource))
        {
            return;
        }
        pResource->SetRefCount(pResource->GetRefCount() - 1);
        if (pResource->GetRefCount() <= 0)
        {
            m_toDelete.push(pResource);
            RemoveResource(pResource);
            m_toUpload.removeAll(pResource);

            if (pResource->GetType().compare("article") == 0)
            {
                m_order.removeAll(pResource->GetId());
            }
        }
        else
        {
            m_toUpload.push(pResource);
        }
    }

    bool BuilderResourceManifest::UpdateArticleOrder(const QString& id, bool direction, ErrorCode& error)
    {
        int index = m_order.indexOf(id);
        if (index == -1)
        {
            m_syncUpdateCallback(QString("Couldn't find article: %1").arg(id), LogError);
            error = ErrorCode::MissingArticle;
            return false;
        }
        m_order.removeAll(id);

        index = index + (direction ? -1 : 1);
        if (index < 0)
        {
            index = 0;
        }
        if (index > m_order.count())
        {
            index = m_order.count();
        }
        m_order.insert(index, id);
        return true;
    }

    EndpointManager* BuilderResourceManifest::GetEndpointManager() const
    {
        return m_endpointManager;
    }

    void BuilderResourceManifest::Sync()
    {
        if (!m_endpointManager->GetSelectedEndpoint())
        {
            FailSync(ErrorCode::NoEndpoint);
            return;
        }
        if (!InitS3Connector())
        {
            FailSync(ErrorCode::S3Fail);
            return;
        }
        ResourceManifest::Sync();
    }

    void BuilderResourceManifest::Reset()
    {
        if (s_syncing)
        {
            m_syncUpdateCallback("Sync is already running", LogError);
            return;
        }

        m_toUpload.clear();

        for (auto pResource : m_toDelete)
        {
            delete pResource;
        }
        m_toDelete.clear();

        m_uidGenerator->Clear();

        ResourceManifest::Reset();
    }

    void BuilderResourceManifest::PersistLocalResources()
    {
        // we are switching to another manifest here, while having the local data still present
        // so we need to explicitly mark it for uploading so that it does not get deleted
        for (auto pResource : m_resources)
        {
            if (!m_toUpload.contains(pResource))
            {
                m_toUpload.push(pResource);
            }
        }
    }

    void BuilderResourceManifest::SetSyncType(SyncType syncType)
    {
        m_syncType = syncType;
    }

    bool BuilderResourceManifest::HasChanges() const
    {
        return m_toUpload.count() > 0 || m_toDelete.count() > 0;
    }

    void BuilderResourceManifest::AppendResource(Resource* pResource)
    {
        m_uidGenerator->AddUid(pResource->GetId().toInt());
        ResourceManifest::AppendResource(pResource);
    }

    void BuilderResourceManifest::RemoveResource(Resource* pResource)
    {
        m_uidGenerator->RemoveUid(pResource->GetId().toInt());
        ResourceManifest::RemoveResource(pResource);
    }

    void BuilderResourceManifest::OnDownloadFail()
    {
        QMessageBox msgBox(QMessageBox::Critical,
            "Sync failed",
            QString("%1\n\n%2")
            .arg(GetErrorMessage(ErrorCode::ManifestDownloadFail))
            .arg(QObject::tr("Overwrite resource manifest?")),
            QMessageBox::Yes | QMessageBox::No);
        if (msgBox.exec() == QMessageBox::Yes)
        {
            if (!UploadManifest())
            {
                m_failed = true;
                m_errorCode = ErrorCode::ManifestUploadFail;
            }
        }
        ResourceManifest::OnDownloadFail();
    }

    //! This function overrides ResourceManifest Read and tries to do some minimal version checking
    //! NOTE: version checking is not done yet, this needs a lot more work to version check properly
    ErrorCode BuilderResourceManifest::Read(const QJsonObject& json)
    {
        int version = json["version"].toInt();
        if (version > m_version && m_syncType == SyncType::Verify)
        {
            return ErrorCode::OutOfSync;
        }

        m_version = version;
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
                // local resource is outdated, keep its in delete list, and download new one instead
                if (pNewResource->GetVersion() > pOldResource->GetVersion())
                {
                    if (m_syncType == SyncType::Merge)
                    {
                        m_toDownload.push(pNewResource);
                    }
                    else if (m_syncType == SyncType::Overwrite)
                    {
                        m_toDelete.push(pNewResource);
                    }
                }
                // local resource is newer or same version, keep it (remove from toDelete list)
                // and don't need to download new one
                else
                {
                    delete pNewResource;
                    toDelete.removeAll(pOldResource);
                }
            }
            else
            {
                // if remote resource was NOT deleted locally then download it
                if (!FindById(pNewResource->GetId(), m_toDelete))
                {
                    if (m_syncType == SyncType::Merge)
                    {
                        m_toDownload.push(pNewResource);
                    }
                    else if (m_syncType == SyncType::Overwrite)
                    {
                        m_toDelete.push(pNewResource);
                    }
                }
                // otherwise user deleted it... needs version check here
                else
                {
                    delete pNewResource;
                }
            }
        }

        for (auto pResource : m_toUpload)
        {
            toDelete.removeAll(pResource);
        }

        for (auto pResource : toDelete)
        {
            RemoveResource(pResource);
            m_toDelete.removeAll(pResource);
            delete pResource;
        }

        //! Sync article display order
        //! this is more complex to implement because articles may have been added or deleted
        //! remotely by another developer while newsbuilder was running
        //! for now just overwrite s3 version
        QJsonArray orderArray = json["order"].toArray();
        for (auto idObject : orderArray)
        {
            QString id = idObject.toString();
            if (FindById(id, m_toDownload) && !m_order.contains(id))
            {
                m_order.append(id);
            }
        }
        return ErrorCode::None;
    }

    //! Write resource manifest to JSON
    void BuilderResourceManifest::Write(QJsonObject& json) const
    {
        QJsonArray resourceArray;
        foreach(auto pResource, m_resources)
        {
            QJsonObject resourceObject;
            pResource->Write(resourceObject);
            resourceArray.append(resourceObject);
        }
        json["resources"] = resourceArray;

        QJsonArray orderArray;
        foreach(auto id, m_order)
        {
            orderArray.append(id);
        }
        json["order"] = orderArray;
        json["version"] = m_version;
    }

    //! Figure out how many resources need to be synced
    void BuilderResourceManifest::PrepareForSync()
    {
        // if resource is locally marked for deletion, then we don't need to upload/download it
        for (auto pResource : m_toDelete)
        {
            m_toDownload.removeAll(pResource);
            m_toUpload.removeAll(pResource);
        }

        m_syncLeft =
            m_toDownload.size() +
            m_toUpload.size() +
            m_toDelete.size();
    }

    void BuilderResourceManifest::SyncResources()
    {
        ResourceManifest::SyncResources();
        UploadResources();
        DeleteResources();
    }

    void BuilderResourceManifest::UploadResources()
    {
        QStack<Resource*> failures;
        while (m_toUpload.count() > 0)
        {
            m_syncUpdateCallback(
                QString("Uploading: %1 resources left").arg(m_toUpload.count()),
                LogInfo);

            auto pResource = m_toUpload.pop();

            auto uploadDescriptor = UploadDescriptor(*pResource);
            uploadDescriptor.Upload(*m_s3Connector,
                [&](QString url)
                {
                    UpdateSync();
                },
                [&, pResource](QString error)
                {
                    failures.push(pResource);
                    m_failed = true;
                    UpdateSync();
                    m_syncUpdateCallback(
                        error,
                        LogError);
                });
        }
        while (failures.count() > 0)
        {
            m_toUpload.push(failures.pop());
        }
    }

    void BuilderResourceManifest::DeleteResources()
    {
        while (m_toDelete.count() > 0)
        {
            m_syncUpdateCallback(
                QString("Deleting: %1 resources left").arg(m_toDelete.count()),
                LogInfo);

            auto pResource = m_toDelete.pop();

            auto deleteDescriptor = DeleteDescriptor(*pResource);
            deleteDescriptor.Delete(*m_s3Connector,
                [&, pResource]()
                {
                    delete pResource;
                    UpdateSync();
                },
                [&, pResource](QString error)
                {
                    m_failed = true;
                    delete pResource;
                    UpdateSync();
                    m_syncUpdateCallback(
                        QString("Failed to delete resource: %1").arg(error),
                        LogError);
                });
        }
    }

    void BuilderResourceManifest::FinishSync()
    {
        if (!m_failed)
        {
            QString error;
            if (!UploadManifest())
            {
                m_failed = true;
                m_errorCode = ErrorCode::ManifestUploadFail;
            }
        }
        ResourceManifest::FinishSync();
    }

    bool BuilderResourceManifest::UploadManifest()
    {
        m_syncUpdateCallback(QObject::tr("%1 to %2")
            .arg("Uploading manifest")
            .arg(m_endpointManager->GetSelectedEndpoint()->GetBucket()),
            LogInfo);
        m_version++;
        Aws::String awsError;
        QJsonObject manifestObject;
        Write(manifestObject);
        QJsonDocument doc(manifestObject);
        auto ss = Aws::MakeShared<Aws::StringStream>(
            S3Connector::ALLOCATION_TAG,
            std::ios::in | std::ios::out | std::ios::binary);
        *ss << QString(doc.toJson(QJsonDocument::Compact)).toStdString();
        Aws::String url;
        if (!m_s3Connector->PutObject("resourceManifest", ss, url, awsError))
        {
            m_syncUpdateCallback(QObject::tr(awsError.c_str()), LogError);
            return false;
        }
        return true;
    }

    bool BuilderResourceManifest::InitS3Connector() const
    {
        auto awsProfileName =
            m_endpointManager->GetSelectedEndpoint()->GetAwsProfile();
        auto bucketName =
            m_endpointManager->GetSelectedEndpoint()->GetBucket();
        Aws::String awsError;
        if (!m_s3Connector->Init(
            awsProfileName.toStdString().c_str(),
            bucketName.toStdString().c_str(),
            awsError))
        {
            m_syncUpdateCallback(QObject::tr(awsError.c_str()), LogError);
            return false;
        }
        return true;
    }

}
