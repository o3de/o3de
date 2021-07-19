/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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


