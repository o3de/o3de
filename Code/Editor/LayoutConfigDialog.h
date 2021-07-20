/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_LAYOUTCONFIGDIALOG_H
#define CRYINCLUDE_EDITOR_LAYOUTCONFIGDIALOG_H

#pragma once

#if !defined(Q_MOC_RUN)
#include "LayoutWnd.h"

#include <QDialog>
#endif

namespace Ui {
    class CLayoutConfigDialog;
}

class LayoutConfigModel;

// CLayoutConfigDialog dialog

class CLayoutConfigDialog
    : public QDialog
{
    Q_OBJECT

public:
    CLayoutConfigDialog(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CLayoutConfigDialog();

    void SetLayout(EViewLayout layout);
    EViewLayout GetLayout() const { return m_layout; };

    // Dialog Data
protected:
    virtual void OnOK();

    LayoutConfigModel* m_model;
    EViewLayout m_layout;
    QScopedPointer<Ui::CLayoutConfigDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_LAYOUTCONFIGDIALOG_H
