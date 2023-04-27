/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QUrl>
#include <QString>
#include <QStringList>
#endif

namespace O3DE::ProjectManager
{
    class ProjectInfo
    {
    public:
        ProjectInfo() = default;

        ProjectInfo(
            const QString& path,
            const QString& projectName,
            const QString& displayName,
            const QString& id,
            const QString& origin,
            const QString& summary,
            const QString& iconPath,
            const QString& newPreviewImagePath,
            const QString& newBackgroundImagePath,
            bool needsBuild);
                    
        bool operator==(const ProjectInfo& rhs) const;
        bool operator!=(const ProjectInfo& rhs) const;

        bool IsValid() const;
        const QString& GetProjectDisplayName() const;

        //! IMPORTANT this path might be the project folder or
        //! the path to a remote project.json file in the cache
        QString m_path;

        //! From project.json
        QString m_projectName;
        QString m_displayName;
        QString m_version;
        QString m_engineName;
        QString m_enginePath;
        QString m_id;
        QString m_origin;
        QString m_summary;
        QString m_iconPath;
        QString m_requirements;
        QString m_license;
        QStringList m_userTags;

        QStringList m_requiredGemDependencies;
        QStringList m_optionalGemDependencies;

        // Used as temp variable for replace images
        QString m_newPreviewImagePath;
        QString m_newBackgroundImagePath;

        bool m_remote = false;

        //! Used in project creation
        bool m_needsBuild = false; //! Does this project need to be built
        bool m_buildFailed = false;
        QUrl m_logUrl;
    };
} // namespace O3DE::ProjectManager
