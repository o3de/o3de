/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
// #include <ISystem.h>
#include <QtWidgets/QWidget>
#endif

namespace Ui
{
    class InterpreterWindow;
}

namespace ScriptCanvasEditor
{
    class InterpreterWindow
        : public QWidget
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(InterpreterWindow, AZ::SystemAllocator, 0);

        explicit InterpreterWindow(QWidget* parent = nullptr);

    private:
        AZStd::unique_ptr<Ui::InterpreterWindow> m_view;
    };
}
