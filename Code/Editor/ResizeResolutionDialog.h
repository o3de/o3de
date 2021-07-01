/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_RESIZERESOLUTIONDIALOG_H
#define CRYINCLUDE_EDITOR_RESIZERESOLUTIONDIALOG_H

#pragma once
// ResizeResolutionDialog.h : header file
//

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui {
    class CResizeResolutionDialog;
}

class ResizeResolutionModel;

/////////////////////////////////////////////////////////////////////////////
// CResizeResolutionDialog dialog

class CResizeResolutionDialog
    : public QDialog
{
    // Construction
public:
    CResizeResolutionDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CResizeResolutionDialog();

    void SetSize(uint32 dwSize);
    uint32 GetSize();

private:
    ResizeResolutionModel* m_model;
    QScopedPointer<Ui::CResizeResolutionDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_RESIZERESOLUTIONDIALOG_H
