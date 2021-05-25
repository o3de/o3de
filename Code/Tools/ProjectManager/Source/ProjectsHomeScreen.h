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
#endif

namespace O3DE::ProjectManager
{
    class ProjectsHomeScreen
        : public ScreenWidget
    {

    public:
        explicit ProjectsHomeScreen(QWidget* parent = nullptr);
        ~ProjectsHomeScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

    protected slots:
        void HandleNewProjectButton();
        void HandleAddProjectButton();
        void HandleOpenProject(const QString& projectPath);
        void HandleEditProject(const QString& projectPath);
        void HandleEditProjectGems(const QString& projectPath);
        void HandleCopyProject(const QString& projectPath);
        void HandleRemoveProject(const QString& projectPath);
        void HandleDeleteProject(const QString& projectPath);

    private:
        QAction* m_createNewProjectAction;
        QAction* m_addExistingProjectAction;

        const QString m_projectPreviewImagePath = "/preview.png";
        inline constexpr static int s_contentMargins = 80;
        inline constexpr static int s_spacerSize = 20;
        inline constexpr static int s_projectButtonRowCount = 4;
        inline constexpr static int s_newProjectButtonWidth = 156;

    };

} // namespace O3DE::ProjectManager
