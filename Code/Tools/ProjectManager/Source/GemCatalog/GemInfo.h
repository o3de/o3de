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
            O3DEFoundation = 1 << 0,
            Local = 1 << 1,
            NumGemOrigins = 2
        };
        Q_DECLARE_FLAGS(GemOrigins, GemOrigin)
        static QString GetGemOriginString(GemOrigin origin);

        GemInfo() = default;
        GemInfo(const QString& name, const QString& creator, const QString& summary, Platforms platforms, bool isAdded);
        bool IsPlatformSupported(Platform platform) const;

        bool IsValid() const;

        QString m_path;
        QString m_name;
        QString m_displayName;
        QString m_creator;
        GemOrigin m_gemOrigin = Local;
        bool m_isAdded = false; //! Is the gem currently added and enabled in the project?
        QString m_summary;
        Platforms m_platforms;
        Types m_types; //! Asset and/or Code and/or Tool
        QStringList m_features;
        QString m_directoryLink;
        QString m_documentationLink;
        QString m_version;
        QString m_lastUpdatedDate;
        int m_binarySizeInKB = 0;
        QStringList m_dependingGemUuids;
        QStringList m_conflictingGemUuids;
    };
} // namespace O3DE::ProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::Platforms)
Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::Types)
Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::GemOrigins)
