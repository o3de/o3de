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
#endif

class QTreeView;

namespace Ui
{
    class KinematicDescriptionDialog;
}

namespace PhysX
{
    namespace Editor
    {
        /// Dialog for explaining the difference between Simulated and Kinematic bodies.
        class KinematicDescriptionDialog : public QDialog
        {
            Q_OBJECT

        public:
            explicit KinematicDescriptionDialog(bool kinematicSetting, QWidget* parent = nullptr);
            ~KinematicDescriptionDialog();

            bool GetResult() const;

        private:
            void OnButtonClicked();
            void InitializeButtons();
            void UpdateDialogText();

            QScopedPointer<Ui::KinematicDescriptionDialog> m_ui;
            bool m_kinematicSetting = false;
        };
    } // namespace Editor
} // namespace PhysX

