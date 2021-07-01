/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void initializeControls();
    void initializeItemViews();

signals:
    void reloadCSS();

private:
    Ui::MainWidget *ui;
};

#endif // MAINWIDGET_H
