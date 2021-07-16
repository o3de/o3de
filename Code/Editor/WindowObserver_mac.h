/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <objc/objc.h>
#include <QObject>

class QWindow;

namespace Editor
{
    class WindowObserver
        : public QObject
    {
        Q_OBJECT
    public:
        explicit WindowObserver(QWindow* window, QObject* parent);

        virtual ~WindowObserver();

    public slots:
        void setWindowIsMoving(bool isMoving);
        void setWindowIsResizing(bool isResizing);

    signals:
        void windowIsMovingOrResizingChanged(bool isMovingOrResizing);

    private:
        bool m_windowIsMoving;
        bool m_windowIsResizing;

        id m_windowObserver;
    };
} // namespace editor
