/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QWidget>
#include <QScopedPointer>

class QHideEvent;
class QMainWindow;
class QShowEvent;
class QToolBar;

namespace Ui {
    class ToolBarPage;
}

class ToolBarPage
    : public QWidget
{
    Q_OBJECT

public:
    explicit ToolBarPage(QMainWindow* parent = nullptr);
    ~ToolBarPage() override;

protected:
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QScopedPointer<Ui::ToolBarPage> ui;
    QMainWindow* m_mainWindow;
    QToolBar* m_toolBar;

    void refreshIconSizes();
};
