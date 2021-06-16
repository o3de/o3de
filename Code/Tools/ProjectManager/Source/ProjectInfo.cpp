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

#include <ProjectInfo.h>
#include <ProjectManagerDefs.h>

#include <QDir>

namespace O3DE::ProjectManager
{
    ProjectInfo::ProjectInfo(
        const QString& path,
        const QString& projectName,
        const QString& displayName,
        const QString& origin,
        const QString& summary,
        const QString& iconPath,
        const QString& imagePath,
        const QString& backgroundImagePath,
        bool needsBuild)
        : m_path(path)
        , m_projectName(projectName)
        , m_displayName(displayName)
        , m_origin(origin)
        , m_summary(summary)
        , m_iconPath(iconPath)
        , m_imagePath(imagePath)
        , m_backgroundImagePath(backgroundImagePath)
        , m_needsBuild(needsBuild)
    {
    }

    bool ProjectInfo::operator==(const ProjectInfo& rhs) const
    {
        if (m_path != rhs.m_path)
        {
            return false;
        }
        if (m_projectName != rhs.m_projectName)
        {
            return false;
        }
        if (m_displayName != rhs.m_displayName)
        {
            return false;
        }
        if (m_origin != rhs.m_origin)
        {
            return false;
        }
        if (m_summary != rhs.m_summary)
        {
            return false;
        }
        if (m_iconPath != rhs.m_iconPath)
        {
            return false;
        }
        if (m_imagePath != rhs.m_imagePath)
        {
            return false;
        }
        if (m_backgroundImagePath != rhs.m_backgroundImagePath)
        {
            return false;
        }

        return true;
    }

    bool ProjectInfo::operator!=(const ProjectInfo& rhs) const
    {
        return !operator==(rhs);
    }

    bool ProjectInfo::IsValid() const
    {
        return !m_path.isEmpty() && !m_projectName.isEmpty();
    }
} // namespace O3DE::ProjectManager
