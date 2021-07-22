/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        const QString& newPreviewImagePath,
        const QString& newBackgroundImagePath,
        bool needsBuild)
        : m_path(path)
        , m_projectName(projectName)
        , m_displayName(displayName)
        , m_origin(origin)
        , m_summary(summary)
        , m_iconPath(iconPath)
        , m_newPreviewImagePath(newPreviewImagePath)
        , m_newBackgroundImagePath(newBackgroundImagePath)
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
        if (m_newPreviewImagePath != rhs.m_newPreviewImagePath)
        {
            return false;
        }
        if (m_newBackgroundImagePath != rhs.m_newBackgroundImagePath)
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

    const QString& ProjectInfo::GetProjectDisplayName() const
    {
        if (!m_displayName.isEmpty())
        {
            return m_displayName;
        }
        else
        {
            return m_projectName;
        }
    }
} // namespace O3DE::ProjectManager
