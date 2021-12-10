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
#endif

namespace O3DE::ProjectManager
{
    class EngineInfo
    {
    public:
        EngineInfo() = default;
        EngineInfo(const QString& path, const QString& name, const QString& version, const QString& thirdPartyPath);

        // from engine.json
        QString m_version;
        QString m_name;
        QString m_thirdPartyPath;

        QString m_path;

        // from o3de_manifest.json
        QString m_defaultProjectsFolder;
        QString m_defaultGemsFolder;
        QString m_defaultTemplatesFolder;
        QString m_defaultRestrictedFolder;

        bool m_registered = false;

        bool IsValid() const;
    };
} // namespace O3DE::ProjectManager
