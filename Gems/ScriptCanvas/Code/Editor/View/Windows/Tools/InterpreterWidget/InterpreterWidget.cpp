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
        AZ::SerializeContext* serializeContext = nullptr;

        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "InterpreterWidget::InterpreterWidget Failed to retrieve serialize context.");
        m_view->setupUi(this);

        AzToolsFramework::ReflectedPropertyEditor* propertyEditor = nullptr;
        propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(this);
        propertyEditor->setObjectName("InterpreterWidget::ReflectedPropertyEditor");
        propertyEditor->Setup(serializeContext, this, true, 250);
        propertyEditor->show();
        propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        propertyEditor->AddInstance(&m_interpreter, azrtti_typeid(m_interpreter));
        propertyEditor->InvalidateAll();
        propertyEditor->ExpandAll();
        m_view->interpreterLayout->insertWidget(0, propertyEditor, 0);
        
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

    void InterpreterWidget::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Interpreter>()
                ->Field("interpreter", &InterpreterWidget::m_interpreter)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<InterpreterWidget>("Script Canvas Interpreter Widget", "A Widget for a ScriptCanvas Interpreted")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &InterpreterWidget::m_interpreter, "Interpreter", "Interpreter")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
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
}

