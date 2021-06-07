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
#include <ProjectSettingsScreen.h>
#endif

namespace O3DE::ProjectManager
{
    class UpdateProjectSettingsScreen
        : public ProjectSettingsScreen
    {
    public:
        explicit UpdateProjectSettingsScreen(QWidget* parent = nullptr);
        ~UpdateProjectSettingsScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        void SetProjectInfo(const ProjectInfo& projectInfo);

    protected:
        bool ValidateProjectPath() override;
    };

} // namespace O3DE::ProjectManager
