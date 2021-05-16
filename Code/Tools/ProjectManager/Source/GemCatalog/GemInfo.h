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

        GemInfo() = default;
        GemInfo(const QString& name, const QString& creator, const QString& summary, Platforms platforms, bool isAdded);

        bool IsValid() const;

        QString m_path;
        QString m_name;
        QString m_displayName;
        AZ::Uuid m_uuid;
        QString m_creator;
        bool m_isAdded = false; //! Is the gem currently added and enabled in the project?
        QString m_summary;
        Platforms m_platforms;
        QStringList m_features;
        QString m_version;
        QString m_lastUpdatedDate;
        QString m_documentationUrl;
        QVector<AZ::Uuid> m_dependingGemUuids;
        QVector<AZ::Uuid> m_conflictingGemUuids;
    };
} // namespace O3DE::ProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::Platforms)
