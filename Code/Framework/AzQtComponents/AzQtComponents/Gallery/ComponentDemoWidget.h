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
#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#include <QScopedPointer>
#endif

namespace Ui {
    class ComponentDemoWidget;
}

class QMenu;

class ComponentDemoWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit ComponentDemoWidget(QWidget* parent = nullptr);
    ~ComponentDemoWidget() override;

Q_SIGNALS:
    void styleChanged(bool enableLegacyUI);
    void refreshStyle();

private:
    void addPage(QWidget* widget, const QString& title);
    void setupMenuBar();
    void createEditMenuPlaceholders();

    QScopedPointer<Ui::ComponentDemoWidget> ui;
    QMenu* m_editMenu = nullptr;
};


