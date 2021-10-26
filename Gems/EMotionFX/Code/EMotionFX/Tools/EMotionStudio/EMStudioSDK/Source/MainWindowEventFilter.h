/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QAbstractNativeEventFilter>

namespace EMStudio
{
    class MainWindow;

    class NativeEventFilter
        : public QAbstractNativeEventFilter
    {
    public:
        NativeEventFilter(MainWindow* mainWindow)
            : QAbstractNativeEventFilter(),
            m_mainWindow(mainWindow)
        {
        }

        virtual bool nativeEventFilter(const QByteArray& /*eventType*/, void* message, long* /*result*/) Q_DECL_OVERRIDE;

    private:
        [[maybe_unused]] MainWindow* m_mainWindow;
    };
}
