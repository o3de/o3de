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
#include <ScreenWidget.h>
#include <ProjectInfo.h>
#endif

namespace Ui
{
    class ProjectSettingsClass;
}

namespace O3DE::ProjectManager
{
    class ProjectSettingsScreen
        : public ScreenWidget
    {
    public:
        explicit ProjectSettingsScreen(QWidget* parent = nullptr);
        ~ProjectSettingsScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        ProjectInfo GetProjectInfo();
        void SetProjectInfo();

        bool Validate();

    protected slots:
        void HandleGemsButton();

    private:
        QScopedPointer<Ui::ProjectSettingsClass> m_ui;
    };

} // namespace O3DE::ProjectManager
