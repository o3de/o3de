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
        ProjectInfo(const QString& path, const QString& projectName, const QString& productName, const AZ::Uuid projectId);

        // from o3de_manifest.json and o3de_projects.json
        QString m_path;

        // from project.json
        QString m_projectName;
        QString m_productName;
        AZ::Uuid m_projectId;
    };
} // namespace O3DE::ProjectManager
