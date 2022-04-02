/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Windows/Tools/InterpreterWidget/InterpreterWindow.h>
#include <Editor/View/Windows/Tools/InterpreterWidget/ui_InterpreterDialog.h>

#include <Editor/View/Windows/Tools/InterpreterWidget/InterpreterWidget.h>

namespace ScriptCanvasEditor
{
    InterpreterWindow::InterpreterWindow(QWidget* parent)
        : QWidget(parent)
        , m_view(new Ui::InterpreterWindow())
    {
        m_view->setupUi(this);
        auto interpreterWidget = new InterpreterWidget(this);
        interpreterWidget->show();
        m_view->layoutForInterpretedWidget->insertWidget(0, interpreterWidget);
        m_view->verticalLayoutWidget->show();
    }
}

