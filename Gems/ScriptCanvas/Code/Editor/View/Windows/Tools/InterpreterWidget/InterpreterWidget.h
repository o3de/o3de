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
#include <Editor/Framework/Interpreter.h>
#include <QtWidgets/QWidget>
#endif

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
    class InstanceDataNode;
}

namespace AZ
{
    class SerializeContext;
}

namespace Ui
{
    class InterpreterWidget;
}

namespace ScriptCanvasEditor
{
    class Configuration;

    /// <summary>
    /// Reusable Editor Widget that provides an editable [Configuration](@ref Configuration) and control for an
    /// [Interpreter](@ref Interpreter). This allows developers to place the widget in almost any tool to provide in-place access to
    /// executing ScriptCanvas graphs.
    /// </summary>
    class InterpreterWidget final
        : public AzQtComponents::StyledDialog
        , public AzToolsFramework::IPropertyEditorNotify
    {
        Q_OBJECT

    public:
        AZ_TYPE_INFO(InterpreterWidget, "{3D2FAD9B-47C0-494A-9BE0-57C14820B40F}");
        AZ_CLASS_ALLOCATOR(InterpreterWidget, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        explicit InterpreterWidget(QWidget* parent = nullptr);

        ~InterpreterWidget() override;

    private:
        Interpreter m_interpreter;
        Ui::InterpreterWidget* m_view = nullptr;
        AZ::EventHandler<const Interpreter&> m_onIterpreterStatusChanged;
        AZ::EventHandler<const Configuration&> m_handlerSourceCompiled;

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
