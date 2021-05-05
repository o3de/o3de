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
#include <QVector>
#endif

namespace O3DE::ProjectManager
{
    class GemInfo
    {
    public:
        enum Platform
        {
            ANDROID = 0x0,
            IOS = 0x1,
            LINUX = 0x2,
            WINDOWS = 0x3,
        };
        Q_DECLARE_FLAGS(Platforms, Platform)

        enum Feature
        {
            Animation = 0x0,
            Assets = 0x1,
            Audio = 0x2,
            Camera = 0x3,
            CloudServices = 0x4,
            Debug = 0x5,
            Environment = 0x6,
            Framework = 0x7,
            Gameplay = 0x8,
            Input = 0x9,
            Material = 0x10,
            Multiplayer = 0x11,
            Physics = 0x12,
            Prototyping = 0x13,
            Rendering = 0x14,
            Required = 0x15,
            Sample = 0x16,
            Scripting = 0x17,
            SDK = 0x18,
            Security = 0x19,
            Social = 0x20,
            Tools = 0x21,
            UI = 0x22,
            Utilities = 0x23,
            UX = 0x24,
            Video = 0x25,
            VR = 0x26,
            Weather = 0x27,
            Core = 0x28
        };
        Q_DECLARE_FLAGS(Features, Feature)

        GemInfo(const QString& name, const QString& creator, const QString& summary, Platforms platforms, bool isAdded);

        QString m_name;
        QString m_displayName;
        AZ::Uuid m_uuid;
        QString m_creator;
        bool m_isAdded = false; //! Is the gem currently added and enabled in the project?
        QString m_summary;
        Platforms m_platforms;
        Features m_features;
        QString m_version;
        QString m_lastUpdatedDate;
        QString m_documentationUrl;
        QVector<AZ::Uuid> m_dependingGemUuids;
        QVector<AZ::Uuid> m_conflictingGemUuids;
    };
} // namespace O3DE::ProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::Platforms)
Q_DECLARE_OPERATORS_FOR_FLAGS(O3DE::ProjectManager::GemInfo::Features)
