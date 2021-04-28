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

#ifndef CRYINCLUDE_EDITOR_FBXEXPORTERDIALOG_H
#define CRYINCLUDE_EDITOR_FBXEXPORTERDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "Resource.h"

#include <QDialog>
#endif

namespace Ui
{
    class FBXExporterDialog;
}

class CFBXExporterDialog
    : public QDialog
{
    Q_OBJECT
public:
    CFBXExporterDialog (bool bDisplayOnlyFPSSetting = false, QWidget* pParent = nullptr);
    ~CFBXExporterDialog();

    float GetFPS() const;
    bool GetExportCoordsLocalToTheSelectedObject() const;
    bool GetExportOnlyPrimaryCamera() const;
    void SetExportLocalCoordsCheckBoxEnable(bool checked);

    int exec() override;

protected:
    void OnFPSChange();
    void SetExportLocalToTheSelectedObjectCheckBox();
    void SetExportOnlyPrimaryCameraCheckBox();

    void accept() override;

    bool m_bDisplayOnlyFPSSetting;
private:
    QScopedPointer<Ui::FBXExporterDialog> m_ui;
};
#endif // CRYINCLUDE_EDITOR_FBXEXPORTERDIALOG_H
