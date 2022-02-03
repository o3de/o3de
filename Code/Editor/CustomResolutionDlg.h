/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : A dialog for getting a resolution info from users

//  Notice     : Refer to ViewportTitleDlg.cpp for a use case.


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
    Q_OBJECT
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
