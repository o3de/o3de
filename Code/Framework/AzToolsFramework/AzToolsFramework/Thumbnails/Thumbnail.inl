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
        template <typename ThumbnailType, typename HasherType, typename EqualKey>
        ThumbnailCache<ThumbnailType, HasherType, EqualKey>::ThumbnailCache()
            : m_thumbnailSize(0)
        {
            BusConnect();
        }

        template <typename ThumbnailType, typename HasherType, typename EqualKey>
        ThumbnailCache<ThumbnailType, HasherType, EqualKey>::~ThumbnailCache()
        {
            BusDisconnect();
        }

        template <typename ThumbnailType, typename HasherType, typename EqualKey>
        void ThumbnailCache<ThumbnailType, HasherType, EqualKey>::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
        {
            for (auto& kvp : m_cache)
            {
                kvp.second->UpdateTime(deltaTime);
            }
        }

        template <typename ThumbnailType, typename HasherType, typename EqualKey>
        bool ThumbnailCache<ThumbnailType, HasherType, EqualKey>::GetThumbnail(
            SharedThumbnailKey key, SharedThumbnail& thumbnail)
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

        template <typename ThumbnailType, typename HasherType, typename EqualKey>
        void ThumbnailCache<ThumbnailType, HasherType, EqualKey>::SetThumbnailSize(int thumbnailSize)
        {
            m_thumbnailSize = thumbnailSize;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework
