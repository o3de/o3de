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
#endif

namespace O3DE::ProjectManager
{
    class TemplateInfo
    {
    public:
        TemplateInfo() = default;
        TemplateInfo(const TemplateInfo& templateInfo) = default;
        TemplateInfo(const QString& path);
        virtual ~TemplateInfo() = default;

        virtual bool IsValid() const;

        QString m_displayName;
        QString m_name;
        QString m_path;
        QString m_summary;
        QString m_requirements;
        QString m_license;
        QStringList m_canonicalTags;
        QStringList m_userTags;
        bool m_isRemote = false;
    };
} // namespace O3DE::ProjectManager
