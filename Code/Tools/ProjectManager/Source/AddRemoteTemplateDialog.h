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

QT_FORWARD_DECLARE_CLASS(QDialogButtonBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(FormLineEditWidget)

    class AddRemoteTemplateDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit AddRemoteTemplateDialog(QWidget* parent = nullptr);
        ~AddRemoteTemplateDialog() = default;

        QString GetRepoPath();

    private:
        void SetDialogReady(bool isReady);

    private slots:
        void ValidateURI();
        void AddTemplateSource();

    private:
        ProjectInfo m_currentProject;

        FormLineEditWidget* m_repoPath = nullptr;

        QDialogButtonBox* m_dialogButtons = nullptr;
        QPushButton* m_applyButton = nullptr;

        QTimer* m_inputTimer = nullptr;
    };
} // namespace O3DE::ProjectManager
