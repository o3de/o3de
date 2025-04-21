/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Core/QtEditorApplication.h>

namespace Editor
{
    class EditorQtApplicationWindows : public EditorQtApplication
    {
        Q_OBJECT
    public:
        EditorQtApplicationWindows(int& argc, char** argv)
            : EditorQtApplication(argc, argv)
        {
        }

        // QAbstractNativeEventFilter:
        bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;

        bool eventFilter(QObject* object, QEvent* event) override;
    };
} // namespace Editor
