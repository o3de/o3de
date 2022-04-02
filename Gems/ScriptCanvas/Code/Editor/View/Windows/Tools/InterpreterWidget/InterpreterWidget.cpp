/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Windows/Tools/InterpreterWidget/InterpreterWidget.h>
#include <Editor/View/Windows/Tools/InterpreterWidget/ui_InterpreterWidget.h>

#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>

namespace ScriptCanvasEditor
{
    InterpreterWidget::InterpreterWidget(QWidget* parent)
        : AzQtComponents::StyledDialog(parent)
        , m_view(new Ui::InterpreterWidget())
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(m_serializeContext, "InterpreterWidget::InterpreterWidget Failed to retrieve serialize context.");
        m_view->setupUi(this);
        m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(this);
        m_propertyEditor->setObjectName("InterpreterWidget::ReflectedPropertyEditor");
        m_propertyEditor->Setup(m_serializeContext, this, true, 250);
        m_propertyEditor->show();
        m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_view->interpreterLayout->insertWidget(0, m_propertyEditor, 0);
        m_view->verticalLayoutWidget->show();
        m_view->buttonStart->show();
        m_view->buttonStop->show();

        m_propertyEditor->AddInstance(&m_interpreter, azrtti_typeid(m_interpreter));
        m_propertyEditor->InvalidateAll();
        m_propertyEditor->ExpandAll();
    }

    // IPropertyEditorNotify
    void InterpreterWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
    {}

    void InterpreterWidget::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* /*node*/, const QPoint& /*point*/)
    {}

    void InterpreterWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
    {}

    void InterpreterWidget::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/)
    {}

    void InterpreterWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/)
    {}

    void InterpreterWidget::SealUndoStack()
    {}
}

