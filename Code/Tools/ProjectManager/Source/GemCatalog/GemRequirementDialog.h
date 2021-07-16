/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>

#include <QDialogButtonBox>
#endif

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(GemModel)

    class GemRequirementDialog
        : public QDialog
    {
        Q_OBJECT // AUTOMOC
    public:
        explicit GemRequirementDialog(GemModel* model, const QVector<QModelIndex>& gemsToAdd, QWidget *parent = nullptr);
        ~GemRequirementDialog() = default;

        QDialogButtonBox::ButtonRole GetButtonResult();

    private:
        void CancelButtonPressed();
        void ContinueButtonPressed();

        QDialogButtonBox::ButtonRole m_buttonResult = QDialogButtonBox::RejectRole;
    };
} // namespace O3DE::ProjectManager
