/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QList>
#include <QStack>
#include <functional>

#include "NewsShared/LogType.h"
#include "NewsShared/ErrorCodes.h"

class QJsonObject;

namespace News
{
    class ArticleDescriptor;
    class UidGenerator;
    class S3Connector;
    class QtDownloadManager;
    class Descriptor;
    class DownloadDescriptor;
    class Resource;

    //! ResourceManifest manages resources.
    /*!
        Manifest contains information on resources, it handles syncing resources with s3
    */
    class ResourceManifest
    {
    public:
        //! ResourceManifest ctor
        /*!
            \param syncSuccessCallback - called once when everything is synced
            \param syncFailCallback - called once when sync failed
            \param syncUpdateCallback - called multiple times to update information on sync process
        */
        explicit ResourceManifest(
            std::function<void()> syncSuccessCallback,
            std::function<void(ErrorCode)> syncFailCallback,
            std::function<void(QString, LogType)> syncUpdateCallback);
        virtual ~ResourceManifest();

        //! Find a resource that matches id
        /*!
            \retval Resource * - a pointer to a Resource with matching id, if none found return nullptr
        */
        Resource* FindById(const QString& id) const;
        static Resource* FindById(const QString& id, const QList<Resource*>& resources);
        static Resource* FindById(const QString& id, const QStack<Resource*>& resources);

        //! Sync resources with s3
        /*
            1) First download resource manifest file
            2) Parse manifest
            3) Determine which resources need to be downloaded, updated, or deleted
            4) Download missing resources or resource that are out of date
            5) Call m_syncSuccessCallback
        */
        virtual void Sync();

        //! Gracegully stop sync process
        /*!
            Aborting works differently depending at what point during sync porocess it is called
            If called before resources started to download, then skip download altogether
            Otherwise gracefully abort all downloads and call m_syncFailCallback
        */
        void Abort();

        //! Called when switching endpoints to reset resource manifest to a clean state
        virtual void Reset();

        QList<Resource*>::const_iterator begin() const;
        QList<Resource*>::const_iterator end() const;

        //! Get order of article resources, so they can be displayed properly in ArticleViewContainer
        QList<QString> GetOrder() const;

    protected:
        //! The root location of cloudfront resources
        QString m_url = "https://lumberyard-data.amazon.com/";
        //! Name of resourceManifest file that links all other resources
        static const QString MANIFEST_NAME;
        //! Identifies whether syncing is in progress
        static bool s_syncing;
        //! Manifest Version
        int m_version = -1;
        //! Number of resources left to sync
        int m_syncLeft = 0;
        //! Identifies whether sync process was aborted
        bool m_aborted = false;
        //! Indentifies whether sync process has failed
        bool m_failed = false;
        ErrorCode m_errorCode = ErrorCode::None;

        QtDownloadManager* m_downloader = nullptr;

        QList<Resource*> m_resources;
        QList<QString> m_order;
        QStack<Resource*> m_toDownload;

        std::function<void()> m_syncSuccessCallback;
        std::function<void(ErrorCode)> m_syncFailCallback;
        std::function<void(QString, LogType)> m_syncUpdateCallback;

        //! Parse resource manifest json, and figure out which resources need to be downloaded
        virtual ErrorCode Read(const QJsonObject& json);

        //! Executed before sync to figure out how many resources need to be synced
        virtual void PrepareForSync();
        //! Actual sync function
        virtual void SyncResources();
        //! Check whether everything is synced, if so call ResourceManifest::FinishSync
        void UpdateSync();
        //! Notify that everything is synced
        virtual void FinishSync();
        void FailSync(ErrorCode error);

        virtual void AppendResource(Resource* pResource);
        virtual void RemoveResource(Resource* pResource);

        virtual void OnDownloadSuccess(QByteArray data);
        virtual void OnDownloadFail();
        virtual void DownloadResources();

    private:
        void ReadConfig();
        void DeleteResources();
    };
} // namespace News
