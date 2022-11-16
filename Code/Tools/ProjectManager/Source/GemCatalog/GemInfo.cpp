/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GemInfo.h"

#include <QObject>

namespace O3DE::ProjectManager
{
    GemInfo::GemInfo(const QString& name, const QString& creator, const QString& summary, Platforms platforms, bool isAdded)
        : m_name(name)
        , m_origin(creator)
        , m_summary(summary)
        , m_platforms(platforms)
        , m_isAdded(isAdded)
    {
    }
    
    bool GemInfo::IsValid() const
    {
        return !m_name.isEmpty() && !m_path.isEmpty();
    }

    QString GemInfo::GetPlatformString(Platform platform)
    {
        switch (platform)
        {
        case Android:
            return QObject::tr("Android");
        case iOS:
            return QObject::tr("iOS");
        case Linux:
            return QObject::tr("Linux");
        case macOS:
            return QObject::tr("macOS");
        case Windows:
            return QObject::tr("Windows");
        default:
            return QObject::tr("<Unknown Platform>");
        }
    }

    QString GemInfo::GetTypeString(Type type)
    {
        switch (type)
        {
        case Asset:
            return QObject::tr("Asset");
        case Code:
            return QObject::tr("Code");
        case Tool:
            return QObject::tr("Tool");
        default:
            return QObject::tr("<Unknown Type>");
        }
    }

    QString GemInfo::GetGemOriginString(GemOrigin origin)
    {
        switch (origin)
        {
        case Open3DEngine:
            return QObject::tr("Open 3D Engine");
        case Local:
            return QObject::tr("Local");
        case Remote:
            return QObject::tr("Remote");
        default:
            return QObject::tr("<Unknown Gem Origin>");
        }
    }

    QString GemInfo::GetDownloadStatusString(DownloadStatus status)
    {
        switch (status)
        {
        case NotDownloaded:
            return QObject::tr("Not Downloaded");
        case Downloading:
            return QObject::tr("Downloading");
        case Downloaded:
            return QObject::tr("Downloaded");
        case UnknownDownloadStatus:
        default:
            return QObject::tr("<Unknown Download Status>");
        }
    };

    bool GemInfo::IsPlatformSupported(Platform platform) const
    {
        return (m_platforms & platform);
    }

    bool GemInfo::operator<(const GemInfo& gemInfo) const
    {
        return (m_displayName < gemInfo.m_displayName);
    }

    GemInfo::Platforms GemInfo::GetPlatformFromString(const QString& platformText)
    {
        if(platformText == "Windows")
        {
            return GemInfo::Platform::Windows;
        }
        else if(platformText == "Linux")
        {
            return GemInfo::Platform::Linux;
        }
        else if(platformText == "Android")
        {
            return GemInfo::Platform::Android;
        }
        else if(platformText == "iOS")
        {
            return GemInfo::Platform::iOS;
        }
        else if(platformText == "macOS")
        {
            return GemInfo::Platform::macOS;
        }
        else
        {
            return GemInfo::Platforms();
        }
    }

    GemInfo::Platforms GemInfo::GetPlatformsFromStringList(const QStringList& platformStrings)
    {
        GemInfo::Platforms newPlatforms = GemInfo::Platforms();
        for(const QString& platform : platformStrings)
        {
            newPlatforms |= GetPlatformFromString(platform);
        }
        return newPlatforms;
    }

    QStringList GemInfo::GetPlatformsAsStringList() const
    {
        QStringList platformStrings;
        for (int i = 0; i < GemInfo::NumPlatforms; ++i)
        {
            const GemInfo::Platform platform = static_cast<GemInfo::Platform>(1 << i);
            if(m_platforms & platform)
            {
                platformStrings << GetPlatformString(platform);
            }
        }
        return platformStrings;
    }
} // namespace O3DE::ProjectManager
