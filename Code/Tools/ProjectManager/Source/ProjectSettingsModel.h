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
    class ProjectSettingsModel
    {
    public:
        ProjectSettingsModel(
            const QString& projectName = "", const QString& projectLocation = "", const QString& imageLocation = "",
            const QString& backgroundImageLocation = "", bool isNew = true);

        QString m_projectName;
        QString m_projectLocation;
        QString m_imageLocation;
        QString m_backgroundImageLocation;
        bool m_isNew = true; //! Is this a new project or existing
    };
} // namespace O3DE::ProjectManager
