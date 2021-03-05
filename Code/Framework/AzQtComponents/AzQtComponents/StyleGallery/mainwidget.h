/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
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
