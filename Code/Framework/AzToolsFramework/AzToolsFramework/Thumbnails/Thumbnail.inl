/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QSharedPointer>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        template <class ThumbnailType, class Hasher, class EqualKey>
        ThumbnailCache<ThumbnailType, Hasher, EqualKey>::ThumbnailCache()
        {
            BusConnect();
        }

        template <class ThumbnailType, class Hasher, class EqualKey>
        ThumbnailCache<ThumbnailType, Hasher, EqualKey>::~ThumbnailCache()
        {
            BusDisconnect();
        }

        template <class ThumbnailType, class Hasher, class EqualKey>
        void ThumbnailCache<ThumbnailType, Hasher, EqualKey>::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
        {
            for (auto& kvp : m_cache)
            {
                kvp.second->UpdateTime(deltaTime);
            }
        }

        template <class ThumbnailType, class Hasher, class EqualKey>
        bool ThumbnailCache<ThumbnailType, Hasher, EqualKey>::GetThumbnail(SharedThumbnailKey key, SharedThumbnail& thumbnail)
        {
            auto it = m_cache.find(key);
            if (it != m_cache.end())
            {
                thumbnail = it->second;
                return true;
            }
            if (IsSupportedThumbnail(key))
            {
                thumbnail = QSharedPointer<ThumbnailType>(new ThumbnailType(key));
                m_cache[key] = thumbnail;
                return true;
            }
            return false;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework
