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

#include <QDialog>
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QDialogButtonBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(FormLineEditWidget)

    class AddRemoteProjectDialog
        : public QDialog
    {
    public:
        explicit AddRemoteProjectDialog(QWidget* parent = nullptr);
        ~AddRemoteProjectDialog() = default;

        QString GetRepoPath();
        QString GetInstallPath();
        bool ShouldBuild();

        void SetCurrentProject(const ProjectInfo& projectInfo);

    private:
        void SetDialogReady(bool isReady);


        ProjectInfo m_currentProject;

        FormLineEditWidget* m_repoPath = nullptr;
        FormLineEditWidget* m_installPath = nullptr;

        QCheckBox* m_autoBuild = nullptr;

        QLabel* m_buildToggleLabel = nullptr;

        QLabel* m_downloadProjectLabel = nullptr;

        QLabel* m_requirementsTitleLabel = nullptr;
        QLabel* m_licensesTitleLabel = nullptr;

        QLabel* m_requirementsContentLabel = nullptr;
        QLabel* m_licensesContentLabel = nullptr;

        QDialogButtonBox* m_dialogButtons = nullptr;
        QPushButton* m_applyButton = nullptr;
    };
} // namespace O3DE::ProjectManager
