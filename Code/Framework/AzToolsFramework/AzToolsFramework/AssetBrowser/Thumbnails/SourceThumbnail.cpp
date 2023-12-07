/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/Results.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <QString>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        // SourceThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        SourceThumbnailKey::SourceThumbnailKey(const AZ::Uuid& sourceUuid)
            : ThumbnailKey()
            , m_sourceUuid(sourceUuid)
        {
        }

        const AZ::Uuid& SourceThumbnailKey::GetSourceUuid() const
        {
            return m_sourceUuid;
        }

        size_t SourceThumbnailKey::GetHash() const
        {
            return m_sourceUuid.GetHash();
        }

        bool SourceThumbnailKey::Equals(const ThumbnailKey* other) const
        {
            if (!ThumbnailKey::Equals(other))
            {
                return false;
            }
            return m_sourceUuid == azrtti_cast<const SourceThumbnailKey*>(other)->GetSourceUuid();
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceThumbnail
        //////////////////////////////////////////////////////////////////////////
        static constexpr const char* DefaultFileIconPath = "Icons/AssetBrowser/Default_16.svg";
        QMutex SourceThumbnail::m_mutex;

        SourceThumbnail::SourceThumbnail(SharedThumbnailKey key)
            : Thumbnail(key)
        {
        }

        void SourceThumbnail::Load()
        {
            m_state = State::Loading;

            auto sourceKey = azrtti_cast<const SourceThumbnailKey*>(m_key.data());
            AZ_Assert(sourceKey, "Incorrect key type, excpected SourceThumbnailKey");

            bool foundIt = false;
            AZStd::string watchFolder;
            AZ::Data::AssetInfo assetInfo;
            AssetSystemRequestBus::BroadcastResult(
                foundIt, &AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID, sourceKey->GetSourceUuid(), assetInfo, watchFolder);

            QString iconPathToUse;
            if (foundIt)
            {
                // note that there might not actually be a UUID for this yet.  So we don't look it up.
                AZ::EBusAggregateResults<SourceFileDetails> results;
                AssetBrowserInteractionNotificationBus::BroadcastResult(results, &AssetBrowserInteractionNotificationBus::Events::GetSourceFileDetails, assetInfo.m_relativePath.c_str());
                // there could be multiple listeners to this, return the first one with actual image path
                auto it = AZStd::find_if(results.values.begin(), results.values.end(), [](const SourceFileDetails& details) { return !details.m_sourceThumbnailPath.empty(); });

                if (it != results.values.end())
                {
                    const char* resultPath = it->m_sourceThumbnailPath.c_str();
                    // its an ordered bus, though, so first one wins.
                    // we have to massage this though.  there are three valid possibilities
                    // 1. its a relative path to source, in which case we have to find the full path
                    // 2. its an absolute path, in which case we use it as-is
                    // 3. its an embedded resource, in which case we use it as is.

                    // is it an embedded resource or absolute path?
                    if ((resultPath[0] == ':') || (!AzFramework::StringFunc::Path::IsRelative(resultPath)))
                    {
                        iconPathToUse = QString::fromUtf8(resultPath);
                    }
                    else
                    {
                        // getting here means its a relative path.  Can we find the real path of the file?  This also searches in gems for sources.
                        foundIt = false;
                        AssetSystemRequestBus::BroadcastResult(foundIt, &AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, resultPath, assetInfo, watchFolder);

                        AZ_WarningOnce("Asset Browser", foundIt, "Unable to find source icon file in any source folders or gems: %s\n", resultPath);

                        if (foundIt)
                        {
                            // the absolute path is join(watchfolder, relativepath); // since its relative to the watch folder.
                            AZStd::string finalPath;
                            AzFramework::StringFunc::Path::Join(watchFolder.c_str(), assetInfo.m_relativePath.c_str(), finalPath);
                            iconPathToUse = QString::fromUtf8(finalPath.c_str());
                        }
                    }
                }
            }

            if (iconPathToUse.isEmpty())
            {
                AZ::IO::FixedMaxPath engineRoot = AZ::Utils::GetEnginePath();
                AZ_Assert(!engineRoot.empty(), "Engine Root not initialized");
                iconPathToUse = (engineRoot / DefaultFileIconPath).c_str();
            }

            m_pixmap.load(iconPathToUse);
            m_state = m_pixmap.isNull() ? State::Failed : State::Ready;
            QueueThumbnailUpdated();
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        SourceThumbnailCache::SourceThumbnailCache()
            : ThumbnailCache<SourceThumbnail>() {}

        SourceThumbnailCache::~SourceThumbnailCache() = default;

        const char* SourceThumbnailCache::GetProviderName() const
        {
            return ProviderName;
        }

        bool SourceThumbnailCache::IsSupportedThumbnail(SharedThumbnailKey key) const
        {
            return azrtti_istypeof<const SourceThumbnailKey*>(key.data());
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Thumbnails/moc_SourceThumbnail.cpp"
