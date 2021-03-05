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
