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
#include <ProjectInfo.h>
#include <ScreenWidget.h>
#endif

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(FormLineEditWidget)
    QT_FORWARD_DECLARE_CLASS(FormBrowseEditWidget)

    class ProjectSettingsScreen
        : public ScreenWidget
    {
    public:
        explicit ProjectSettingsScreen(QWidget* parent = nullptr);
        ~ProjectSettingsScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        ProjectInfo GetProjectInfo();

        bool Validate();

    protected slots:
        virtual bool ValidateProjectName();
        virtual bool ValidateProjectPath();

    protected:
        QString GetDefaultProjectPath();

        QHBoxLayout* m_horizontalLayout;
        QVBoxLayout* m_verticalLayout;
        FormLineEditWidget* m_projectName;
        FormBrowseEditWidget* m_projectPath;
    };

} // namespace O3DE::ProjectManager
