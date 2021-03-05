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

#include <AzCore/EBus/Results.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <QString>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        // SourceThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        SourceThumbnailKey::SourceThumbnailKey(const char* fileName)
            : ThumbnailKey()
            , m_fileName(fileName)
        {
            AzFramework::StringFunc::Path::Split(fileName, nullptr, nullptr, nullptr, &m_extension);
        }

        const AZStd::string& SourceThumbnailKey::GetFileName() const
        {
            return m_fileName;
        }

        const AZStd::string& SourceThumbnailKey::GetExtension() const
        {
            return m_extension;
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceThumbnail
        //////////////////////////////////////////////////////////////////////////
        static constexpr const char* DefaultFileIconPath = "Editor/Icons/AssetBrowser/Default_16.svg";
        QMutex SourceThumbnail::m_mutex;

        SourceThumbnail::SourceThumbnail(SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
        {
        }

        void SourceThumbnail::LoadThread()
        {
            auto sourceKey = azrtti_cast<const SourceThumbnailKey*>(m_key.data());
            AZ_Assert(sourceKey, "Incorrect key type, excpected SourceThumbnailKey");

            // note that there might not actually be a UUID for this yet.  So we don't look it up.
            AZ::EBusAggregateResults<SourceFileDetails> results;
            AssetBrowserInteractionNotificationBus::BroadcastResult(results, &AssetBrowserInteractionNotificationBus::Events::GetSourceFileDetails, sourceKey->GetFileName().c_str());
            // there could be multiple listeners to this, return the first one with actual image path
            auto it = AZStd::find_if(results.values.begin(), results.values.end(), [](const SourceFileDetails& details) { return !details.m_sourceThumbnailPath.empty(); });

            QString iconPathToUse;

            if (it != results.values.end())
            {
                const char* resultPath = it->m_sourceThumbnailPath.c_str();
                // its an ordered bus, though, so first one wins.
                // we have to massage this though.  there are three valid possibilities
                // 1. its a relative path to source, in which case we have to find the full path
                // 2. its an absolute path, in which case we use it as-is
                // 3. its an embedded resource, in which case we use it as is.

                // is it an embedded resource or absolute path?
                if ((resultPath[0] == ':')||(!AzFramework::StringFunc::Path::IsRelative(resultPath)))
                {
                    iconPathToUse = QString::fromUtf8(resultPath);
                }
                else
                {
                    // getting here means its a relative path.  Can we find the real path of the file?  This also searches in gems for sources.
                    bool foundIt = false;
                    AZStd::string watchFolder;
                    AZ::Data::AssetInfo assetInfo;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundIt, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, resultPath, assetInfo, watchFolder);

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
            else
            {
                const char* engineRoot = nullptr;
                AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
                AZ_Assert(engineRoot, "Engine Root not initialized");
                AZStd::string iconPath = AZStd::string::format("%s%s", engineRoot, DefaultFileIconPath);
                iconPathToUse = iconPath.c_str();
            }

            m_icon = QIcon(iconPathToUse);
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        SourceThumbnailCache::SourceThumbnailCache()
            : ThumbnailCache<SourceThumbnail, SourceKeyHash, SourceKeyEqual>() {}

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
