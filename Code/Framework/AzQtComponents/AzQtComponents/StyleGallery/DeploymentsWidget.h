/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DEPLOYMENTSWIDGET_H
#define DEPLOYMENTSWIDGET_H

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/StyleGallery/ui_deploymentswidget.h>

#include <QWidget>
#include <QScopedPointer>
#endif

class DeploymentsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DeploymentsWidget(QWidget *parent = 0);
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void addDeployment();
private:
    QScopedPointer<Ui::DeploymentsWidget> ui;
};

#endif
