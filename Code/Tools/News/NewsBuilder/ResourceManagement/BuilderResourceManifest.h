/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "NewsShared/ResourceManagement/ResourceManifest.h"

#include <QStack>

class QJsonObject;

namespace News
{
    class EndpointManager;
    class ArticleDescriptor;
    class UidGenerator;
    class S3Connector;
    class QtDownloadManager;
    class Descriptor;
    class DownloadDescriptor;
    class Resource;

    //! Type of sync behavior
    enum class SyncType
    {
        Merge, // merge resources from both endpoints, i.e. replacing outdated and appending missing (also normal sync behavior)
        Overwrite, // overwrite resources on new endpoint with old endpoint
        Verify // attempt to publish changes but abort if out of sync
    };

    //! Adds news-builder functionality layer to resource manager
    class BuilderResourceManifest
        : public ResourceManifest
    {
    public:
        explicit BuilderResourceManifest(
            std::function<void()> syncSuccessCallback,
            std::function<void(ErrorCode)> syncFailCallback,
            std::function<void(QString, LogType)> syncUpdateCallback);

        //! Create new article resource
        /*!
            Create new article with default parameters,
            Add it to resource collection,
            Mark it for upload.

            \retval Resource * - a pointer to an article Resource
        */
        Resource* AddArticle();

        //! Create new image resource
        /*!
            Create new image from file,
            add it to resource collection,
            mark it for upload.

            \param filename - path to an image on hard drive
            \retval Resource * - a pointer to an image Resource
        */
        Resource* AddImage(const QString& filename);

        //! If resource was modified, add it to upload list and increment its version
        void UpdateResource(Resource* resource);

        //! When resource is used by several other resources, this function is called
        //! to increment its ref count
        void UseResource(const QString& id);

        //! When resource is no longer used by another resource
        //! decrement refCount,
        //! if nothing else is using it, then mark for delete
        void FreeResource(const QString& id);

        //! Move article either up or down in order queue
        bool UpdateArticleOrder(const QString& id, bool direction, ErrorCode& error);

        EndpointManager* GetEndpointManager() const;

        void Sync() override;

        void Reset() override;

        //! When switching endpoints Merge allows to persist resources to another endpoint,
        //! thus copying news from one location to another upon next Sync
        void PersistLocalResources();

        void SetSyncType(SyncType syncType);

        //! Are there local changes
        bool HasChanges() const;

    protected:
        void AppendResource(Resource* pResource) override;
        void RemoveResource(Resource* pResource) override;
        void OnDownloadFail() override;
        void FinishSync() override;

    private:
        S3Connector* m_s3Connector = nullptr;
        UidGenerator* m_uidGenerator = nullptr;
        EndpointManager* m_endpointManager = nullptr;
        SyncType m_syncType = SyncType::Merge;

        QStack<Resource*> m_toUpload;
        QStack<Resource*> m_toDelete;

        ErrorCode Read(const QJsonObject& json) override;
        void Write(QJsonObject& json) const;

        void PrepareForSync() override;
        void SyncResources() override;

        void UploadResources();
        void DeleteResources();

        bool UploadManifest();

        //! Initializes S3 connector with selected endpoint
        bool InitS3Connector() const;
    };
} // namespace News
