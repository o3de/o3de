/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/TickBus.h>

AZ_PUSH_DISABLE_WARNING(4127 4251 4800, "-Wunknown-warning-option") // 4127: conditional expression is constant
                                                                    // 4251: 'QLocale::d': class 'QSharedDataPointer<QLocalePrivate>' needs to have dll-interface to be used by clients of class 'QLocale'
                                                                    // 4800: 'int': forcing value to bool 'true' or 'false' (performance warning)

#include <QObject>
#include <QPixmap>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //! ThumbnailKey is used to locate thumbnails in thumbnail cache
        /*
            ThumbnailKey contains any kind of identifiable information to retrieve thumbnails (e.g. assetId, assetType, filename, etc.)
            To use thumbnail system, keep reference to your thumbnail key, and retrieve Thumbnail via ThumbnailerRequestBus
        */
        class ThumbnailKey : public QObject
        {
            Q_OBJECT
        public:
            AZ_RTTI(ThumbnailKey, "{43F20F6B-333D-4226-8E4F-331A62315255}");

            ThumbnailKey() = default;
            virtual ~ThumbnailKey() = default;

            void SetReady(bool ready);

            bool IsReady() const;

            virtual bool UpdateThumbnail();

            virtual size_t GetHash() const;

            virtual bool Equals(const ThumbnailKey* other) const;

        Q_SIGNALS:
            //! This signal is sent whenever the thumbnail image where data has changed.
            void ThumbnailUpdated() const;
            //! Force update mapped thumbnails
            void ThumbnailUpdateRequested() const;

        private:
            bool m_ready = false;
        };

        typedef QSharedPointer<ThumbnailKey> SharedThumbnailKey;

#define MAKE_TKEY(type, ...) QSharedPointer<type>(new type(__VA_ARGS__))

        //! Thumbnail is the base class in thumbnailer system.
        /*
            Thumbnail handles storing and updating data for each specific thumbnail
            Thumbnail also emits Updated signal whenever thumbnail data changes, this signal is listened to by every ThumbnailKey that maps
           to this thumbnail Because you should be storing reference to ThumbnailKey and not Thumbnail, connect to ThumbnailKey signal
           instead
        */
        class Thumbnail : public QObject
        {
            Q_OBJECT
        public:
            enum class State
            {
                Unloaded,
                Loading,
                Ready,
                Failed
            };

            Thumbnail(SharedThumbnailKey key);
            ~Thumbnail() override;
            bool operator==(const Thumbnail& other) const;
            virtual void Load();
            const QPixmap& GetPixmap() const;
            SharedThumbnailKey GetKey() const;
            State GetState() const;
            AZStd::chrono::steady_clock::time_point GetLastTimeUpdated() const;
            bool CanAttemptReload() const;

        Q_SIGNALS:
            void ThumbnailUpdated() const;

        public Q_SLOTS:
            virtual void Update() {}

        protected:
            void QueueThumbnailUpdated();

            AZStd::chrono::steady_clock::time_point m_lastTimeUpdated = AZStd::chrono::steady_clock::now();
            AZStd::atomic<State> m_state;
            SharedThumbnailKey m_key;
            QPixmap m_pixmap;
        };

        typedef QSharedPointer<Thumbnail> SharedThumbnail;

        //! Interface to retrieve thumbnails
        class ThumbnailProvider
        {
        public:
            ThumbnailProvider() = default;
            virtual ~ThumbnailProvider() = default;
            virtual bool GetThumbnail(SharedThumbnailKey key, SharedThumbnail& thumbnail) = 0;

            //! Priority identifies ThumbnailProvider order
            //! Higher priority means this ThumbnailProvider will take precedence in generating a thumbnail when a supplied ThumbnailKey is
            //! supported by multiple providers.
            virtual int GetPriority() const
            {
                return 0;
            }
            //! A unique ThumbnailProvider name identifyier
            virtual const char* GetProviderName() const = 0;
        };

        typedef QSharedPointer<ThumbnailProvider> SharedThumbnailProvider;
    } // namespace Thumbnailer
} // namespace AzToolsFramework

namespace AZStd
{
    // hash specialization
    template<>
    struct hash<AzToolsFramework::Thumbnailer::SharedThumbnailKey>
    {
        AZ_FORCE_INLINE size_t operator()(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const
        {
            return key->GetHash();
        }
    };

    template<>
    struct equal_to<AzToolsFramework::Thumbnailer::SharedThumbnailKey>
    {
        AZ_FORCE_INLINE bool operator()(
            const AzToolsFramework::Thumbnailer::SharedThumbnailKey& left,
            const AzToolsFramework::Thumbnailer::SharedThumbnailKey& right) const
        {
            return left->Equals(right.data());
        }
    };
} // namespace AZStd

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //! ThumbnailCache manages thumbnails of specific type, derive your custom provider from this
        /*
            ThumbnailType - type of thumbnails managed
            Hasher - hashing function for storing thumbnail keys in the hashtable
            EqualKey - equality function for storing thumbnail keys in the hashtable
            Hasher and EqualKey need to be provided on individual basis depending on
            what constitutes a unique key and how should the key collection be optimized
        */
        template<class ThumbnailType, class Hasher = AZStd::hash<SharedThumbnailKey>, class EqualKey = AZStd::equal_to<SharedThumbnailKey>>
        class ThumbnailCache : public ThumbnailProvider
        {
        public:
            ThumbnailCache();
            ~ThumbnailCache() override;

            bool GetThumbnail(SharedThumbnailKey key, SharedThumbnail& thumbnail) override;

        protected:
            AZStd::unordered_map<SharedThumbnailKey, SharedThumbnail, Hasher, EqualKey> m_cache;

            //! Check if thumbnail key is handled by this provider, overload in derived class
            virtual bool IsSupportedThumbnail(SharedThumbnailKey key) const = 0;
        };

        #define MAKE_TCACHE(cacheType, ...) QSharedPointer<cacheType>(new cacheType(__VA_ARGS__))
    } // namespace Thumbnailer
} // namespace AzToolsFramework

Q_DECLARE_METATYPE(AzToolsFramework::Thumbnailer::SharedThumbnailKey)

#include <AzToolsFramework/Thumbnails/Thumbnail.inl>
