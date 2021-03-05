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
