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

#include <QSharedPointer>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        template <class ThumbnailType, class Hasher, class EqualKey>
        ThumbnailCache<ThumbnailType, Hasher, EqualKey>::ThumbnailCache()
            : m_thumbnailSize(0)
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
                thumbnail = QSharedPointer<ThumbnailType>(new ThumbnailType(key, m_thumbnailSize));
                m_cache[key] = thumbnail;
                return true;
            }
            return false;
        }

        template <class ThumbnailType, class Hasher, class EqualKey>
        void ThumbnailCache<ThumbnailType, Hasher, EqualKey>::SetThumbnailSize(int thumbnailSize)
        {
            m_thumbnailSize = thumbnailSize;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework
