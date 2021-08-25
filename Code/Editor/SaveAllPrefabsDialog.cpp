/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #include <AzCore/PlatformDef.h>
 #include "SaveAllPrefabsDialog.h"
 #include <AzQtComponents/Components/Widgets/CheckBox.h>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_SaveAllPrefabsDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

SaveAllPrefabsDialog::SaveAllPrefabsDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SaveAllPrefabsDialog)
{
    ui->setupUi(this);
    AzQtComponents::CheckBox::applyToggleSwitchStyle(ui->saveAllPrefabsCheckBox);
    AzQtComponents::CheckBox::applyToggleSwitchStyle(ui->rememberPrefabSavePreferenceCheckBox);
}

SaveAllPrefabsDialog::~SaveAllPrefabsDialog()
{
    delete ui;
}
