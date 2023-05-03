/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMetaType>
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
            DownloadSuccessful,
            DownloadFailed,
            Downloaded
        };
        static QString GetDownloadStatusString(DownloadStatus status);

        static Platforms GetPlatformFromString(const QString& platformText);

        static Platforms GetPlatformsFromStringList(const QStringList& platformStrings);

        GemInfo() = default;
        GemInfo(const QString& name, const QString& creator, const QString& summary, Platforms platforms, bool isAdded);
        bool IsPlatformSupported(Platform platform) const;
        QString GetNameWithVersionSpecifier(const QString& comparator = "==") const;
        bool IsValid() const;
        bool IsCompatible() const;

        bool operator<(const GemInfo& gemInfo) const;

        QStringList GetPlatformsAsStringList() const;

        QString m_path;
        QString m_name = "Unknown Gem Name";
        QString m_displayName;
        QString m_origin = "Unknown Creator";
        GemOrigin m_gemOrigin = Local;
        QString m_originURL;
        QString m_iconPath;
        bool m_isAdded = false; //! Is the gem explicitly added (not a dependency) and enabled in the project?
        bool m_isEngineGem = false;
        bool m_isProjectGem = false;
        QString m_summary = "No summary provided.";
        Platforms m_platforms;
        Types m_types; //! Asset and/or Code and/or Tool
        DownloadStatus m_downloadStatus = UnknownDownloadStatus;
        QStringList m_features;
        QString m_requirement;
        QString m_licenseText;
        QString m_licenseLink;
        QString m_directoryLink;
        QString m_documentationLink;
        QString m_repoUri;
        QString m_version = "Unknown Version";
        QString m_lastUpdatedDate = "Unknown Date";
        int m_binarySizeInKB = 0;
        QStringList m_dependencies;
        QStringList m_compatibleEngines;
        QStringList m_incompatibleEngineDependencies; //! Specific to the current project's engine 
        QStringList m_incompatibleGemDependencies; //! Specific to the current project and engine
        QString m_downloadSourceUri;
        QString m_sourceControlUri;
        QString m_sourceControlRef;
    };
} // namespace O3DE::ProjectManager

Q_DECLARE_METATYPE(O3DE::ProjectManager::GemInfo);

Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::Platforms);
Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::Types);
Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::GemOrigins);

