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
#include <AzQtComponents/Components/StyledDialog.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QtWidgets/QWidget>
#endif

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
    class InstanceDataNode;
}

namespace Ui
{
    class InterpreterWidget;
}

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvasEditor
{
    class Interpreter;

    class InterpreterWidget
        : public AzQtComponents::StyledDialog
        , public AzToolsFramework::IPropertyEditorNotify
    {
        Q_OBJECT

    public:
        AZ_TYPE_INFO(InterpreterWidget, "{3D2FAD9B-47C0-494A-9BE0-57C14820B40F}");
        AZ_CLASS_ALLOCATOR(InterpreterWidget, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        explicit InterpreterWidget(QWidget* parent = nullptr);

    private:
        AZStd::unique_ptr<Interpreter> m_interpreter;
        AZStd::unique_ptr<Ui::InterpreterWidget> m_view;
        AZ::EventHandler<const Interpreter&> m_onIterpreterStatusChanged;

        void OnButtonStartPressed();
        void OnButtonStopPressed();
        void OnInterpreterStatusChanged(const Interpreter&);

        // IPropertyEditorNotify ...
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode*) {}
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode*) {}
        void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint&) {}
        void SealUndoStack() {}
        void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode*) {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode*) {}
        // ... IPropertyEditorNotify
    };
}
