/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog.h>

#include <QDialogButtonBox>
#endif

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(FormLineEditWidget)

    class GemRepoAddDialog
        : public QDialog
    {
    public:
        explicit GemRepoAddDialog(QWidget* parent = nullptr);
        ~GemRepoAddDialog() = default;

        QDialogButtonBox::ButtonRole GetButtonResult();
        QString GetRepoPath();

    private:
        void CancelButtonPressed();
        void ContinueButtonPressed();

        FormLineEditWidget* m_repoPath = nullptr;

        QDialogButtonBox::ButtonRole m_buttonResult = QDialogButtonBox::RejectRole;
    };
} // namespace O3DE::ProjectManager
