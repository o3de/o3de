/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //////////////////////////////////////////////////////////////////////////
        // SourceControlThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        SourceControlThumbnailKey::SourceControlThumbnailKey(const char* fileName)
            : ThumbnailKey()
            , m_fileName(fileName)
            , m_updateInterval(4)
        {
            m_nextUpdate = AZStd::chrono::steady_clock::now();
        }

        const AZStd::string& SourceControlThumbnailKey::GetFileName() const
        {
            return m_fileName;
        }

        bool SourceControlThumbnailKey::UpdateThumbnail()
        {
            if (!IsReady() || !SourceControlThumbnail::ReadyForUpdate())
            {
                return false;
            }

            const auto now(AZStd::chrono::steady_clock::now());
            if (m_nextUpdate >= now)
            {
                return false;
            }
            m_nextUpdate = now + m_updateInterval;
            emit ThumbnailUpdateRequested();
            return true;
        }

        bool SourceControlThumbnailKey::Equals(const ThumbnailKey* other) const
        {
            if (!ThumbnailKey::Equals(other))
            {
                return false;
            }
            return m_fileName == azrtti_cast<const SourceControlThumbnailKey*>(other)->GetFileName();
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceControlThumbnail
        //////////////////////////////////////////////////////////////////////////
        static const char* WRITABLE_ICON_PATH = "Icons/AssetBrowser/Writable_16.svg";
        static const char* NONWRITABLE_ICON_PATH = "Icons/AssetBrowser/NonWritable_16.svg";

        bool SourceControlThumbnail::m_readyForUpdate = true;

        SourceControlThumbnail::SourceControlThumbnail(SharedThumbnailKey key)
            : Thumbnail(key)
        {
            AZ::IO::FixedMaxPath engineRoot = AZ::Utils::GetEnginePath();
            AZ_Assert(!engineRoot.empty(), "Engine Root not initialized");

            m_writableIconPath = (engineRoot / WRITABLE_ICON_PATH).String();
            m_nonWritableIconPath = (engineRoot / NONWRITABLE_ICON_PATH).String();


            BusConnect();
        }

        SourceControlThumbnail::~SourceControlThumbnail()
        {
            BusDisconnect();
        }

        void SourceControlThumbnail::FileStatusChanged(const char* filename)
        {
            // when file status is changed, force instant update
            auto sourceControlKey = azrtti_cast<const SourceControlThumbnailKey*>(m_key.data());
            AZ_Assert(sourceControlKey, "Incorrect key type, excpected SourceControlThumbnailKey");

            AZStd::string myFileName(sourceControlKey->GetFileName());
            AZ::StringFunc::Path::Normalize(myFileName);
            if (AZ::StringFunc::Equal(myFileName.c_str(), filename))
            {
                Update();
            }
        }

        void SourceControlThumbnail::RequestSourceControlStatus()
        {
            auto sourceControlKey = azrtti_cast<const SourceControlThumbnailKey*>(m_key.data());
            AZ_Assert(sourceControlKey, "Incorrect key type, excpected SourceControlThumbnailKey");

            bool isSourceControlActive = false;
            SourceControlConnectionRequestBus::BroadcastResult(isSourceControlActive, &SourceControlConnectionRequests::IsActive);
            if (isSourceControlActive && SourceControlCommandBus::FindFirstHandler() != nullptr)
            {
                m_readyForUpdate = false;
                SourceControlCommandBus::Broadcast(&SourceControlCommands::GetFileInfo, sourceControlKey->GetFileName().c_str(),
                    AZStd::bind(&SourceControlThumbnail::SourceControlFileInfoUpdated, this, AZStd::placeholders::_1, AZStd::placeholders::_2));
            }
        }

        void SourceControlThumbnail::SourceControlFileInfoUpdated(bool succeeded, const SourceControlFileInfo& fileInfo)
        {
            if (succeeded)
            {
                if (fileInfo.HasFlag(AzToolsFramework::SCF_Writeable))
                {
                    m_pixmap.load(m_writableIconPath.c_str());
                }
                else
                {
                    m_pixmap.load(m_nonWritableIconPath.c_str());
                }
            }
            else
            {
                m_pixmap = QPixmap();
            }
            m_readyForUpdate = true;
            QueueThumbnailUpdated();
        }

        void SourceControlThumbnail::Update()
        {
            RequestSourceControlStatus();
        }

        bool SourceControlThumbnail::ReadyForUpdate()
        {
            return m_readyForUpdate;
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceControlThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        SourceControlThumbnailCache::SourceControlThumbnailCache()
            : ThumbnailCache<SourceControlThumbnail>()
        {
        }

        SourceControlThumbnailCache::~SourceControlThumbnailCache() = default;

        const char* SourceControlThumbnailCache::GetProviderName() const
        {
            return ProviderName;
        }

        bool SourceControlThumbnailCache::IsSupportedThumbnail(SharedThumbnailKey key) const
        {
            return azrtti_istypeof<const SourceControlThumbnailKey*>(key.data());
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_SourceControlThumbnail.cpp"
