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
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(FormLineEditWidget)

    class AddRemoteProjectDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit AddRemoteProjectDialog(QWidget* parent = nullptr);
        ~AddRemoteProjectDialog() = default;

        QString GetRepoPath();
        QString GetInstallPath();
        bool ShouldBuild();

        void SetCurrentProject(const ProjectInfo& projectInfo);

    private:
        void SetDialogReady(bool isReady);

    signals:
        void StartObjectDownload(const QString& objectName, const QString& destinationPath, bool queueBuild);

    private slots:
        void ValidateURI();
        void DownloadObject();

    private:
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

        QTimer* m_inputTimer = nullptr;
    };
} // namespace O3DE::ProjectManager
