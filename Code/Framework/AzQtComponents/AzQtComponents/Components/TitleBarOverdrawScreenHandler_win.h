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
