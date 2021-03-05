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

#ifndef DEPLOYMENTSWIDGET_H
#define DEPLOYMENTSWIDGET_H

#if !defined(Q_MOC_RUN)
#include "StyleGallery/ui_deploymentswidget.h"

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
