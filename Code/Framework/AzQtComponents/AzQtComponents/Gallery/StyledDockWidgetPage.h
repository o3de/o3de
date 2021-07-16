/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QScopedPointer>
#include <QPointer>
#endif

namespace Ui {
    class StyledDockWidgetPage;
}

namespace AzQtComponents
{
    class DockMainWindow;
}

class StyledDockWidgetPage : public QWidget
{
    Q_OBJECT

public:
    explicit StyledDockWidgetPage(QWidget* parent = nullptr);
    ~StyledDockWidgetPage() override;

private slots:
    void showMainWindow();

private:
    QScopedPointer<Ui::StyledDockWidgetPage> ui;
    QPointer<AzQtComponents::DockMainWindow> m_mainWindow;
};
