/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ProjectSettingsScreen.h>
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace O3DE::ProjectManager
{
    class UpdateProjectSettingsScreen
        : public ProjectSettingsScreen
    {
    public:
        explicit UpdateProjectSettingsScreen(QWidget* parent = nullptr);
        ~UpdateProjectSettingsScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        ProjectInfo GetProjectInfo() override;
        void SetProjectInfo(const ProjectInfo& projectInfo);

        bool Validate() override;

        void ResetProjectPreviewPath();

    public slots:
        void UpdateProjectPreviewPath();
        void PreviewPathChanged();

    protected:
        bool ValidateProjectPath() override;
        virtual bool ValidateProjectPreview();

        FormBrowseEditWidget* m_projectPreview;
        QLabel* m_projectPreviewImage;

        ProjectInfo m_projectInfo;
        bool m_userChangedPreview; //! Did the user change the project preview path
    };

} // namespace O3DE::ProjectManager
