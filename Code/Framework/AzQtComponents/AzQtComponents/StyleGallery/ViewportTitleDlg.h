/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
