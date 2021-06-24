/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Window/HelpDialog/HelpDialog.h>

namespace MaterialEditor
{
    HelpDialog::HelpDialog(QWidget* parent)
        : QDialog(parent)
        , m_ui(new Ui::HelpDialogWidget)
    {
        m_ui->setupUi(this);
    }

    HelpDialog::~HelpDialog() = default;
} // namespace MaterialEditor

#include <Source/Window/HelpDialog/moc_HelpDialog.cpp>
