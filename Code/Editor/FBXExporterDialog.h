/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
