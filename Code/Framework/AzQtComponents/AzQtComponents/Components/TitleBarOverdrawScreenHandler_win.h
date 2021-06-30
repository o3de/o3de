/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>

#include <QObject>
#endif

class QWindow;
class QScreen;
class QDockWidget;

namespace AzQtComponents
{

class TitleBarOverdrawScreenHandler : public QObject
{
    Q_OBJECT // AUTOMOC
public:
    explicit TitleBarOverdrawScreenHandler(QWindow* window, QObject* parent = nullptr);
    explicit TitleBarOverdrawScreenHandler(QDockWidget* dockWidget, QObject* parent = nullptr);

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWindow* m_window = nullptr;
    QDockWidget* m_dockWidget = nullptr;
    QScreen* m_screen = nullptr;

    void applyOverdrawMargins();
    void registerWindow(QWindow* window);
    void handleFloatingDockWidget();
};

} // namespace AzQtComponents
