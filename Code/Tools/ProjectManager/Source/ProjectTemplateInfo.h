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
#include <QString>
#include <QStringList>
#endif

namespace O3DE::ProjectManager
{
    class ProjectTemplateInfo
    {
    public:
        ProjectTemplateInfo() = default;
        ProjectTemplateInfo(const QString& path);

        bool IsValid() const;

        QString m_displayName;
        QString m_name;
        QString m_path;
        QString m_summary;
        QStringList m_canonicalTags;
        QStringList m_userTags;
    };
} // namespace O3DE::ProjectManager
