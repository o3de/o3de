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

// Description : A dialog for getting a resolution info from users

//  Notice     : Refer to ViewportTitleDlg.cpp for a use case.


#ifndef CRYINCLUDE_EDITOR_CUSTOMRESOLUTIONDLG_H
#define CRYINCLUDE_EDITOR_CUSTOMRESOLUTIONDLG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui
{
    class CustomResolutionDlg;
}

class CCustomResolutionDlg
    : public QDialog
{
public:
    CCustomResolutionDlg(int w, int h, QWidget* pParent = nullptr);
    ~CCustomResolutionDlg();

    int GetWidth() const;
    int GetHeight() const;

protected:
    void OnInitDialog();

    int m_wDefault, m_hDefault;

    QScopedPointer<Ui::CustomResolutionDlg> m_ui;
};

#endif // CRYINCLUDE_EDITOR_CUSTOMRESOLUTIONDLG_H
