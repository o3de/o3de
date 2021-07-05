/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <QDialog>
#endif


namespace EMotionFX
{
    class JointSelectionWidget;

    class JointSelectionDialog
        : public QDialog
    {
        Q_OBJECT // AUTOMOC

    public:
        JointSelectionDialog(bool singleSelection, const QString& title, const QString& descriptionLabelText, QWidget* parent);
        ~JointSelectionDialog();

        void SelectByJointNames(const AZStd::vector<AZStd::string>& jointNames);
        AZStd::vector<AZStd::string> GetSelectedJointNames() const;

        void SetFilterState(const QString& category, const QString& displayName, bool enabled);
        void HideIcons();

    private slots:
        void OnItemDoubleClicked(const QModelIndex& modelIndex);

    private:
        JointSelectionWidget* m_jointSelectionWidget;
    };
} // namespace EMotionFX
