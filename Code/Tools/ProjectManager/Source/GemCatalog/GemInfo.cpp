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
        , m_creator(creator)
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
} // namespace O3DE::ProjectManager
