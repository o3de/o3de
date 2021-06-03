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

QT_FORWARD_DECLARE_CLASS(QPaintEvent)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)

namespace O3DE::ProjectManager
{
    class ProjectsScreen
        : public ScreenWidget
    {

    public:
        explicit ProjectsScreen(QWidget* parent = nullptr);
        ~ProjectsScreen() = default;

        ProjectManagerScreen GetScreenEnum() override;
        QString GetTabText() override;
        bool IsTab() override;

    protected:
        void NotifyCurrentScreen() override;

    protected slots:
        void HandleNewProjectButton();
        void HandleAddProjectButton();
        void HandleOpenProject(const QString& projectPath);
        void HandleEditProject(const QString& projectPath);
        void HandleCopyProject(const QString& projectPath);
        void HandleRemoveProject(const QString& projectPath);
        void HandleDeleteProject(const QString& projectPath);

        void paintEvent(QPaintEvent* event) override;

    private:
        QFrame* CreateFirstTimeContent();
        QFrame* CreateProjectsContent();
        bool ShouldDisplayFirstTimeContent();

        QAction* m_createNewProjectAction;
        QAction* m_addExistingProjectAction;
        QPixmap m_background;
        QFrame* m_firstTimeContent;
        QFrame* m_projectsContent;
        QStackedWidget* m_stack;

        const QString m_projectPreviewImagePath = "/preview.png";

        inline constexpr static int s_contentMargins = 80;
        inline constexpr static int s_spacerSize = 20;
    };

} // namespace O3DE::ProjectManager
