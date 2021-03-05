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

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

namespace Ui
{
    class ViewportTitleDlg;
}

class ViewportTitleDlg : public QWidget
{
    Q_OBJECT
public:
    ViewportTitleDlg(QWidget* pParent = nullptr);
    virtual ~ViewportTitleDlg();
protected slots:
    void OnMaximize();
    void OnToggleHelpers();
    void OnToggleDisplayInfo();
private:
    void OnInitDialog();
    void SetTitle(const QString &title);

    QString m_title = QLatin1String("Perspective");
    QScopedPointer<Ui::ViewportTitleDlg> m_ui;
};
