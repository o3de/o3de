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
#include <QMutex>
#endif

namespace AzToolsFramework
{
    using namespace Thumbnailer;

    namespace AssetBrowser
    {
        //! SourceAssetBrowserEntry thumbnail key
        class SourceThumbnailKey
            : public ThumbnailKey
        {
            Q_OBJECT
        public:
            AZ_RTTI(SourceThumbnailKey, "{4AF3F33A-4B16-491D-93A5-0CC96DC59814}", ThumbnailKey);

            explicit SourceThumbnailKey(const AZ::Uuid& sourceUuid);
            const AZ::Uuid& GetSourceUuid() const;
            size_t GetHash() const override;
            bool Equals(const ThumbnailKey* other) const override;

        protected:
            AZ::Uuid m_sourceUuid;
        };

        class SourceThumbnail
            : public Thumbnail
        {
            Q_OBJECT
        public:
            SourceThumbnail(SharedThumbnailKey key);
            void Load() override;

        private:
            static QMutex m_mutex;
        };

        //! SourceAssetBrowserEntry thumbnails
        class SourceThumbnailCache
            : public ThumbnailCache<SourceThumbnail>
        {
        public:
            SourceThumbnailCache();
            ~SourceThumbnailCache() override;

            const char* GetProviderName() const override;

            static constexpr const char* ProviderName = "Source Thumbnails";

        protected:
            bool IsSupportedThumbnail(SharedThumbnailKey key) const override;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
