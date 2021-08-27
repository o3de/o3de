/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h>
#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>

#include <QVector>
#include <qnamespace.h>

#endif

class QWindow;
class QPlatformWindow;
class QScreen;

namespace AzQtComponents
{

class TitleBarOverdrawScreenHandler;

class TitleBarOverdrawHandlerWindows
    : public TitleBarOverdrawHandler
{
    Q_OBJECT // AUTOMOC
public:
    TitleBarOverdrawHandlerWindows(QApplication* application, QObject* parent);
    ~TitleBarOverdrawHandlerWindows() override;

    QMargins customTitlebarMargins(HMONITOR monitor, unsigned style, unsigned exStyle, bool maximized, int dpi);

    void applyOverdrawMargins(QWindow* window);

    void polish(QWidget* widget) override;
    void addTitleBarOverdrawWidget(QWidget* widget) override;

private:
    friend class TitleBarOverdrawScreenHandler;

    QPlatformWindow* overdrawWindow(const QVector<QWidget*>& overdrawWidgets, HWND hWnd);
    void applyOverdrawMargins(QPlatformWindow* window, HWND hWnd, bool maximized);

    QVector<QWidget*> m_overdrawWidgets;
    QVector<TitleBarOverdrawScreenHandler*> m_screenHandlers;
    // The below DPI related methods exist in the user32.dll only on Windows 10 version 1607 and beyond and not windows server.  
    // AZQtComopnents is a dependency of AzToolsFramework and linking to them directly will cause a failure at runtime on windows
    // server when loading user32.dll.  We can't base this code on anything at compile time for cases of packaged builds which need to
    // compile executables on windows server for use on windows 10 clients, but also need to process assets themselves for the packaged build
    // The better answer will be to resolve this dependency between aztoolsframework and azqtcomponents.
    HMODULE m_user32Module;
    typedef UINT(WINAPI * PFNGetDpiForWindow) (HWND hwnd);
    PFNGetDpiForWindow m_getDpiForWindowFn{ nullptr };
    typedef BOOL(WINAPI * PFNAdjustWindowRectExForDpi) (LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
    PFNAdjustWindowRectExForDpi m_adjustWindowRectExForDpiFn{ nullptr };
};

} // namespace AzQtComponents
