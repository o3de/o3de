/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ProjectSettingsScreen.h>
#include <ProjectTemplateInfo.h>
#include <QVector>
#endif

QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QFrame)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(TagContainerWidget)
    class NewProjectSettingsScreen
        : public ProjectSettingsScreen
    {
    public:
        explicit NewProjectSettingsScreen(QWidget* parent = nullptr);
        ~NewProjectSettingsScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        QString GetProjectTemplatePath();

        void NotifyCurrentScreen() override;

    private:
        QString GetDefaultProjectPath();
        QFrame* CreateTemplateDetails(int margin);
        void UpdateTemplateDetails(const ProjectTemplateInfo& templateInfo);

        QButtonGroup* m_projectTemplateButtonGroup;
        QLabel* m_templateDisplayName;
        QLabel* m_templateSummary;
        TagContainerWidget* m_templateIncludedGems;
        QVector<ProjectTemplateInfo> m_templates;

        inline constexpr static int s_spacerSize = 20;
        inline constexpr static int s_templateDetailsContentMargin = 20;
    };

} // namespace O3DE::ProjectManager
