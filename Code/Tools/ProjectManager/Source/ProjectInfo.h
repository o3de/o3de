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
#endif

namespace O3DE::ProjectManager
{
    class ProjectInfo
    {
    public:
        ProjectInfo() = default;
        ProjectInfo(const QString& path, const QString& projectName, const QString& displayName,
            const QString& imagePath, const QString& backgroundImagePath, bool isNew);

        bool IsValid() const;

        // from o3de_manifest.json and o3de_projects.json
        QString m_path;

        // From project.json
        QString m_projectName;
        QString m_displayName;

        // Used on projects home screen
        QString m_imagePath;
        QString m_backgroundImagePath;

        // Used in project creation
        bool m_isNew = false; //! Is this a new project or existing
    };
} // namespace O3DE::ProjectManager
