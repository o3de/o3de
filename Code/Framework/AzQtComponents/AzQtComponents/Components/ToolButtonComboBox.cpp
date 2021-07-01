/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/ToolButtonComboBox.h>
#include <QComboBox>

namespace AzQtComponents
{
    ToolButtonComboBox::ToolButtonComboBox(QWidget* parent)
        : ToolButtonWithWidget(new QComboBox(), parent)
        , m_combo(static_cast<QComboBox*>(widget()))
    {
        m_combo->setEditable(true);
    }

    QComboBox* ToolButtonComboBox::comboBox() const
    {
        return m_combo;
    }

} // namespace AzQtComponents

#include "Components/moc_ToolButtonComboBox.cpp"
