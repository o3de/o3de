/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/set.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <QObject>
#include <QList>
#include <QThreadPool>
#endif

class QString;
class QPixmap;

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        class ThumbnailProvider;

        //! ThumbnailContext provides distinct thumbnail location for specific context
        /*
            There can be any number of contexts for every unique feature that may need different types of thumbnails.
            For example 'AssetBrowser' context provides thumbnails specific to Asset Browser
            'PreviewContext' may provide thumbnails for Preview Widget
            'MaterialBrowser' may provide thumbnails for Material Browser
            etc.
        */
        class ThumbnailContext
            : public QObject
            , public ThumbnailContextRequestBus::Handler
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(ThumbnailContext, AZ::SystemAllocator, 0);

            ThumbnailContext();
            ~ThumbnailContext() override;

            //! Is the thumbnail currently loading or is about to load.
            bool IsLoading(SharedThumbnailKey key);
            //! Retrieve thumbnail by key, generate one if needed
            SharedThumbnail GetThumbnail(SharedThumbnailKey key);
            //! Add new thumbnail cache
            void RegisterThumbnailProvider(SharedThumbnailProvider providerToAdd);
            //! Remove thumbnail cache by name if found
            void UnregisterThumbnailProvider(const char* providerName);

            void RedrawThumbnail();

            //! Default context used for most thumbnails
            static constexpr const char* DefaultContext = "Default";

            // ThumbnailContextRequestBus::Handler interface overrides...
            QThreadPool* GetThreadPool() override;

        private:
            struct ProviderCompare {
                bool operator() (const SharedThumbnailProvider& lhs, const SharedThumbnailProvider& rhs) const
                {
                    // sorting in reverse, higher priority means the provider should be considered first
                    return lhs->GetPriority() > rhs->GetPriority();
                }
            };

            //! Collection of thumbnail caches provided by this context
            AZStd::multiset<SharedThumbnailProvider, ProviderCompare> m_providers;
            //! Default missing thumbnail used when no thumbnail for given key can be found within this context
            SharedThumbnail m_missingThumbnail;
            //! Default loading thumbnail used when thumbnail is found by is not yet generated
            SharedThumbnail m_loadingThumbnail;
            //! There is only a limited number of threads on global threadPool, because there can be many thumbnails rendering at once
            //! an individual threadPool is needed to avoid deadlocks
            QThreadPool m_threadPool;
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework
