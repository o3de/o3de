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
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Editor/Framework/Interpreter.h>

#include <AzQtComponents/Components/StyledDialog.h>
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
    class InterpreterWidget
        : public AzQtComponents::StyledDialog
        , public AzToolsFramework::IPropertyEditorNotify
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(InterpreterWidget, AZ::SystemAllocator, 0);

        explicit InterpreterWidget(QWidget* parent = nullptr);

    private:
        AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor = nullptr;
        AZ::SerializeContext* m_serializeContext = nullptr;
        AZStd::unique_ptr<Ui::InterpreterWidget> m_view;
        Interpreter m_interpreter;

        void OnButtonStartPressed();
        void OnButtonStopPressed();
        void ToggleStartStopButtonEnabled();

        // IPropertyEditorNotify ...
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
        void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* /*node*/, const QPoint& /*point*/) override;
        void SealUndoStack() override;
        void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/) override;
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/) override;
        // ... IPropertyEditorNotify
    };
}
