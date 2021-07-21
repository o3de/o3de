/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : A dialog for getting an aspect ratio info from users
// Notice      : Refer to ViewportTitleDlg cpp for a use case


#ifndef CRYINCLUDE_EDITOR_CUSTOMASPECTRATIODLG_H
#define CRYINCLUDE_EDITOR_CUSTOMASPECTRATIODLG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui
{
    class CustomAspectRatioDlg;
}

class CCustomAspectRatioDlg
    : public QDialog
{
    Q_OBJECT
public:
    CCustomAspectRatioDlg(int x, int y, QWidget* pParent = nullptr);
    ~CCustomAspectRatioDlg();

    int GetX() const;
    int GetY() const;

protected:
    void OnInitDialog();

    int m_xDefault, m_yDefault;

    QScopedPointer<Ui::CustomAspectRatioDlg> m_ui;
};

#endif // CRYINCLUDE_EDITOR_CUSTOMASPECTRATIODLG_H
