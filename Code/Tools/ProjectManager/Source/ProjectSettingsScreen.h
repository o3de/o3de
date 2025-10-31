/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ProjectInfo.h>
#include <ScreenWidget.h>

#include <AzCore/Outcome/Outcome.h>
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

        virtual ProjectInfo GetProjectInfo();

        virtual AZ::Outcome<void, QString> Validate() const;

    protected slots:
        virtual void OnProjectNameUpdated();
        virtual void OnProjectPathUpdated();

    protected:
        bool ValidateProjectName() const;
        virtual bool ValidateProjectPath() const;

        QString GetDefaultProjectPath();

        QHBoxLayout* m_horizontalLayout;
        QVBoxLayout* m_verticalLayout;
        FormLineEditWidget* m_projectName;
        FormLineEditWidget* m_projectVersion;
        FormBrowseEditWidget* m_projectPath;
    };

} // namespace O3DE::ProjectManager
