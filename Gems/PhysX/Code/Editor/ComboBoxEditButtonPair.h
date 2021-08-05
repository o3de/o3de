/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QBoxLayout>
#include <QToolButton>
#include <QComboBox>
#include <QEvent>

namespace PhysX
{
    namespace Editor
    {
        /// Wrapper widget for a combo box with a button at the end.
        class ComboBoxEditButtonPair
            : public QWidget
        {
        public:
            explicit ComboBoxEditButtonPair(QWidget* parent);

            QComboBox* GetComboBox();
            QToolButton* GetEditButton();

        private:
            QComboBox* m_comboBox = nullptr;
            QToolButton* m_editButton = nullptr;
        };
    }
}
