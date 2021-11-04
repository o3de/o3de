/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Math/Uuid.h>
#include <QString>
#include <QStringList>
#include <QVector>
#endif

namespace O3DE::ProjectManager
{
    class GemInfo
    {
    public:
        enum Platform
        {
            Android = 1 << 0,
            iOS = 1 << 1,
            Linux = 1 << 2,
            macOS = 1 << 3,
            Windows = 1 << 4,
            NumPlatforms = 5
        };
        Q_DECLARE_FLAGS(Platforms, Platform)
        static QString GetPlatformString(Platform platform);

        enum Type
        {
            Asset = 1 << 0,
            Code = 1 << 1,
            Tool = 1 << 2,
            NumTypes = 3
        };
        Q_DECLARE_FLAGS(Types, Type)
        static QString GetTypeString(Type type);

        enum GemOrigin
        {
            Open3DEngine = 1 << 0,
            Local = 1 << 1,
            Remote = 1 << 2,
            NumGemOrigins = 3
        };
        Q_DECLARE_FLAGS(GemOrigins, GemOrigin)
        static QString GetGemOriginString(GemOrigin origin);

        enum DownloadStatus
        {
            UnknownDownloadStatus = -1,
            NotDownloaded,
            Downloading,
            Downloaded,
        };
        static QString GetDownloadStatusString(DownloadStatus status);

        GemInfo() = default;
        GemInfo(const QString& name, const QString& creator, const QString& summary, Platforms platforms, bool isAdded);
        bool IsPlatformSupported(Platform platform) const;

        bool IsValid() const;

        bool operator<(const GemInfo& gemInfo) const;

        QString m_path;
        QString m_name = "Unknown Gem Name";
        QString m_displayName = "Unknown Gem Name";
        QString m_creator = "Unknown Creator";
        GemOrigin m_gemOrigin = Local;
        bool m_isAdded = false; //! Is the gem currently added and enabled in the project?
        QString m_summary = "No summary provided.";
        Platforms m_platforms;
        Types m_types; //! Asset and/or Code and/or Tool
        DownloadStatus m_downloadStatus = UnknownDownloadStatus;
        QStringList m_features;
        QString m_requirement;
        QString m_directoryLink;
        QString m_documentationLink;
        QString m_version = "Unknown Version";
        QString m_lastUpdatedDate = "Unknown Date";
        int m_binarySizeInKB = 0;
        QStringList m_dependencies;
    };
} // namespace O3DE::ProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::Platforms)
Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::Types)
Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::GemOrigins)
