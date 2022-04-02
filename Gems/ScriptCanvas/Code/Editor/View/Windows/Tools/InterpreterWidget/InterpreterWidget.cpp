/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <Editor/View/Windows/Tools/InterpreterWidget/InterpreterWidget.h>
#include <Editor/View/Windows/Tools/InterpreterWidget/ui_InterpreterWidget.h>

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
        m_propertyEditor->AddInstance(&m_interpreter, azrtti_typeid(m_interpreter));
        m_propertyEditor->InvalidateAll();
        m_propertyEditor->ExpandAll();
        m_view->interpreterLayout->insertWidget(0, m_propertyEditor, 0);
        
        m_view->buttonStart->show();
        m_view->buttonStart->setEnabled(true);
        connect(m_view->buttonStart, &QPushButton::pressed, this, &InterpreterWidget::OnButtonStartPressed);

        m_view->buttonStop->setDisabled(true);
        m_view->buttonStop->show();
        connect(m_view->buttonStop, &QPushButton::pressed, this, &InterpreterWidget::OnButtonStopPressed);
        
        m_view->verticalLayoutWidget->show();
    }

    void InterpreterWidget::OnButtonStartPressed()
    {
        if (m_interpreter.IsExecutable())
        {
            ToggleStartStopButtonEnabled();
            m_interpreter.Execute();
        }
    }

    void InterpreterWidget::OnButtonStopPressed()
    {
        if (m_interpreter.IsExecutable())
        {
            m_interpreter.Stop();
            ToggleStartStopButtonEnabled();
        }
    }

    void InterpreterWidget::ToggleStartStopButtonEnabled()
    {
        if (m_view->buttonStart->isEnabled())
        {
            m_view->buttonStart->setDisabled(true);
            m_view->buttonStop->setEnabled(true);
        }
        else
        {
            m_view->buttonStart->setEnabled(true);
            m_view->buttonStop->setDisabled(true);
        }
    }

    // IPropertyEditorNotify ...
    void InterpreterWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
    {
        AZ_TracePrintf("ScriptCanvas", "InterpreterWidget::AfterPropertyModified(), refreshing configuration");
        // m_interpreter.RefreshConfiguration();
    }

    void InterpreterWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
    {}

    void InterpreterWidget::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* /*node*/, const QPoint& /*point*/)
    {}

    void InterpreterWidget::SealUndoStack()
    {
        AZ_TracePrintf("ScriptCanvas", "InterpreterWidget::SealUndoStack(), refreshing configuration");
        // m_interpreter.RefreshConfiguration();
    }

    void InterpreterWidget::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/)
    {}

    void InterpreterWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/)
    {
        AZ_TracePrintf("ScriptCanvas", "InterpreterWidget::SetPropertyEditingComplete(), refreshing configuration");
        // m_interpreter.RefreshConfiguration();
    }
    // ... IPropertyEditorNotify
}

