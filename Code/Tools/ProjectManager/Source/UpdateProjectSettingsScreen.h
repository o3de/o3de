/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ProjectSettingsScreen.h>
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(FormComboBoxWidget)

    class UpdateProjectSettingsScreen
        : public ProjectSettingsScreen
    {
    public:
        explicit UpdateProjectSettingsScreen(QWidget* parent = nullptr);
        ~UpdateProjectSettingsScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        ProjectInfo GetProjectInfo() override;
        void SetProjectInfo(const ProjectInfo& projectInfo);

        AZ::Outcome<void, QString> Validate() const override;

        void ResetProjectPreviewPath();

    public slots:
        void UpdateProjectPreviewPath();
        void PreviewPathChanged();
        void OnProjectIdUpdated();
        void OnProjectEngineUpdated(int index);

    protected:
        bool ValidateProjectPath() const override;
        virtual bool ValidateProjectPreview() const;
        bool ValidateProjectId() const;

        inline constexpr static int s_collapseButtonSize = 24;

        FormComboBoxWidget* m_projectEngine;
        FormBrowseEditWidget* m_projectPreview;
        QLabel* m_projectPreviewImage;
        FormLineEditWidget* m_projectId;

        QPushButton* m_advancedSettingsCollapseButton = nullptr;
        QWidget* m_advancedSettingWidget = nullptr;

        ProjectInfo m_projectInfo;
        bool m_userChangedPreview; //! Did the user change the project preview path

    protected slots:
        void UpdateAdvancedSettingsCollapseState();
    };

} // namespace O3DE::ProjectManager
