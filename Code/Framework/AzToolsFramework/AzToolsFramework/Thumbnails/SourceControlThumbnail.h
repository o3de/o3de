/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnailBus.h>
#endif

namespace AzToolsFramework
{
    struct SourceControlFileInfo;

    namespace Thumbnailer
    {
        class SourceControlThumbnailKey
            : public ThumbnailKey
        {
            Q_OBJECT
        public:
            AZ_RTTI(SourceControlThumbnailKey, "{F6772100-A178-45A7-8F75-41426B07D829}", ThumbnailKey);

            explicit SourceControlThumbnailKey(const char* fileName);

            const AZStd::string& GetFileName() const;

            bool UpdateThumbnail() override;

            bool Equals(const ThumbnailKey* other) const override;

        protected:
            //! absolute path
            AZStd::string m_fileName;
            //! how often should sc thumbnails auto update
            const AZStd::chrono::minutes m_updateInterval;
            //! time since this sc thumbnail updated
            AZStd::chrono::steady_clock::time_point m_nextUpdate;
        };

        //! SourceControlThumbnail currently replicates the source control functionality within Material Browser
        //! Additionally source control status is refreshed whenever an operation is performed through context menu
        class SourceControlThumbnail
            : public Thumbnail
            , public SourceControlThumbnailRequestBus::Handler
        {
            Q_OBJECT
        public:
            SourceControlThumbnail(SharedThumbnailKey key);
            ~SourceControlThumbnail() override;

            //////////////////////////////////////////////////////////////////////////
            // SourceControlNotificationBus
            //////////////////////////////////////////////////////////////////////////
            void FileStatusChanged(const char* filename) override;

            static bool ReadyForUpdate();

        public Q_SLOTS:
            void Update() override;

        private:
            // To avoid overpopulating sc update stack with status requests, sc thumbnail does not load until this function is called
            void RequestSourceControlStatus();
            void SourceControlFileInfoUpdated(bool succeeded, const SourceControlFileInfo& fileInfo);

            //! If another sc thumbnail is currently requesting sc status this will return false
            static bool m_readyForUpdate;

            AZStd::string m_writableIconPath;
            AZStd::string m_nonWritableIconPath;
        };

        namespace
        {
            class SourceControlKeyHash
            {
            public:
                size_t operator() (const SharedThumbnailKey& /*val*/) const
                {
                    return 0;
                }
            };

            class SourceControlKeyEqual
            {
            public:
                bool operator()(const SharedThumbnailKey& val1, const SharedThumbnailKey& val2) const
                {
                    auto sourceThumbnailKey1 = azrtti_cast<const SourceControlThumbnailKey*>(val1.data());
                    auto sourceThumbnailKey2 = azrtti_cast<const SourceControlThumbnailKey*>(val2.data());
                    if (!sourceThumbnailKey1 || !sourceThumbnailKey2)
                    {
                        return false;
                    }
                    return sourceThumbnailKey1->GetFileName() == sourceThumbnailKey2->GetFileName();
                }
            };
        }

        //! Stores products' thumbnails
        class SourceControlThumbnailCache
            : public ThumbnailCache<SourceControlThumbnail>
        {
        public:
            SourceControlThumbnailCache();
            ~SourceControlThumbnailCache() override;

            const char* GetProviderName() const override;

            static constexpr const char* ProviderName = "SourceControl Thumbnails";

        protected:
            bool IsSupportedThumbnail(SharedThumbnailKey key) const override;
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework
