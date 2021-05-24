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

        // from o3de_manifest.json
        QString m_path;
        QString m_defaultProjectsFolder;
        QString m_defaultGemsFolder;
        QString m_defaultTemplatesFolder;
        QString m_defaultRestrictedFolder;

        bool IsValid() const;
    };
} // namespace O3DE::ProjectManager
