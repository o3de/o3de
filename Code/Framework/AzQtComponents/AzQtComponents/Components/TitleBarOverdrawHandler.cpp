/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <QtGlobal>

#include <QPointer>

#include <QWidget>
#include <QWindow>
#include <QApplication>
#include <QAbstractNativeEventFilter>
#include <QVector>

#ifdef Q_OS_WIN
# include <AzQtComponents/Components/TitleBarOverdrawHandler_win.h>
#endif // Q_OS_WIN


namespace AzQtComponents
{

static QPointer<TitleBarOverdrawHandler> s_titleBarOverdrawHandlerInstance;

TitleBarOverdrawHandler* TitleBarOverdrawHandler::getInstance()
{
    return s_titleBarOverdrawHandlerInstance;
}

TitleBarOverdrawHandler::TitleBarOverdrawHandler(QObject* parent)
    : QObject(parent)
{
    Q_ASSERT(s_titleBarOverdrawHandlerInstance.isNull());
    s_titleBarOverdrawHandlerInstance = this;
}

TitleBarOverdrawHandler::~TitleBarOverdrawHandler()
{
}

#ifndef Q_OS_WIN

TitleBarOverdrawHandler* TitleBarOverdrawHandler::createHandler(QApplication*, QObject* parent)
{
    return new TitleBarOverdrawHandler(parent);
}

#else

TitleBarOverdrawHandler* TitleBarOverdrawHandler::createHandler(QApplication* application, QObject* parent)
{
    return new TitleBarOverdrawHandlerWindows(application, parent);
}

#endif // Q_OS_WIN

} // namespace AzQtComponents
