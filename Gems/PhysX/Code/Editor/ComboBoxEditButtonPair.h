/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
