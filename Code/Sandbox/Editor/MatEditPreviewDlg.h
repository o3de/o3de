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

#ifndef CRYINCLUDE_EDITOR_MATEDITPREVIEWDLG_H
#define CRYINCLUDE_EDITOR_MATEDITPREVIEWDLG_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QScopedPointer>

#include "IDataBaseManager.h"
#endif

class MaterialPreviewModelView;
class QMenuBar;

// MatEditPreviewDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMatEditPreviewDlg dialog
class CMatEditPreviewDlg
    : public QDialog
    , public IDataBaseManagerListener
{
    Q_OBJECT
    // Construction
public:
    CMatEditPreviewDlg(QWidget* parent);   // standard constructor
    ~CMatEditPreviewDlg();

    QSize sizeHint() const override;
    void showEvent(QShowEvent*) override;

    //Functions

    virtual void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event);

protected:
    void SetupMenuBar();

private slots:
    void OnPreviewSphere();
    void OnPreviewPlane();
    void OnPreviewBox();
    void OnPreviewTeapot();
    void OnPreviewCustom();

private:
    QScopedPointer<MaterialPreviewModelView> m_previewCtrl;
    QScopedPointer<QMenuBar> m_menubar;
};

#endif // CRYINCLUDE_EDITOR_MATEDITPREVIEWDLG_H
