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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "FBXPlugin_precompiled.h"
#include "FBXSettingsDlg.h"
#include "FBXExporter.h"
#include "resource.h"

#include <IEditor.h>

#include <QMainWindow>

#include "ui_FBXSettingsDlg.h"

bool OpenFBXSettingsDlg(struct SFBXSettings& settings)
{
    Ui::FBXSettingsDialog ui;
    QDialog dialog(GetIEditor()->GetEditorMainWindow());
    ui.setupUi(&dialog);

    ui.hCopyTextures->setChecked(settings.bCopyTextures);
    ui.hEmbedded->setChecked(settings.bEmbedded);
    ui.hFileFormat->setCurrentIndex(settings.bAsciiFormat ? 1 : 0);
    ui.hConvertAxesForMaxMaya->setChecked(settings.bConvertAxesForMaxMaya);

    if (dialog.exec() != QDialog::Accepted)
        return false;

    settings.bCopyTextures = ui.hCopyTextures->isChecked();
    settings.bEmbedded = ui.hEmbedded->isChecked();
    settings.bAsciiFormat = ui.hFileFormat->currentIndex() == 1;
    settings.bConvertAxesForMaxMaya = ui.hConvertAxesForMaxMaya->isChecked();

    return true;
}
