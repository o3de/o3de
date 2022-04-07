/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Editor/Framework/Interpreter.h>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <Editor/View/Windows/Tools/InterpreterWidget/InterpreterWidget.h>
#include <Editor/View/Windows/Tools/InterpreterWidget/ui_InterpreterWidget.h>

namespace InterpreterWidgetCpp
{
    using namespace ScriptCanvasEditor;

    AZStd::pair<bool, bool> ToStartStopButtonEnabled(InterpreterStatus status)
    {
        switch (status)
        {
        case InterpreterStatus::Ready:
        case InterpreterStatus::Stopped:
            return { true, false };
        case InterpreterStatus::Running:
            return { false, true };

        case InterpreterStatus::Waiting:
        case InterpreterStatus::Misconfigured:
        case InterpreterStatus::Configured:
        case InterpreterStatus::Pending:
        default:
            return { false, false };
        }
    }
}

namespace ScriptCanvasEditor
{
    InterpreterWidget::InterpreterWidget(QWidget* parent)
        : AzQtComponents::StyledDialog(parent)
        , m_view(new Ui::InterpreterWidget())
        , m_interpreter(AZStd::make_unique<Interpreter>())
    {
        m_view->setupUi(this);

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "InterpreterWidget::InterpreterWidget Failed to retrieve serialize context.");
        AzToolsFramework::ReflectedPropertyEditor* propertyEditor = nullptr;
        propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(this);
        propertyEditor->setObjectName("InterpreterWidget::ReflectedPropertyEditor");
        propertyEditor->Setup(serializeContext, this, true, 250);
        propertyEditor->show();
        propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        propertyEditor->AddInstance(m_interpreter.get(), azrtti_typeid<Interpreter>());
        propertyEditor->InvalidateAll();
        propertyEditor->ExpandAll();
        m_view->interpreterLayout->insertWidget(0, propertyEditor, 0);
        connect(m_view->buttonStart, &QPushButton::pressed, this, &InterpreterWidget::OnButtonStartPressed);
        connect(m_view->buttonStop, &QPushButton::pressed, this, &InterpreterWidget::OnButtonStopPressed);

        m_onIterpreterStatusChanged = AZ::EventHandler<const Interpreter&>
            ([this](const Interpreter& interpreter)
            {
                OnInterpreterStatusChanged(interpreter);
            });
        m_onIterpreterStatusChanged.Connect(m_interpreter->GetOnStatusChanged());

        m_handlerSourceCompiled = AZ::EventHandler<const Configuration&>([propertyEditor](const Configuration&)
        {
            propertyEditor->QueueInvalidation(AzToolsFramework::Refresh_EntireTree_NewContent);
        });
        m_interpreter->GetConfiguration().ConnectToSourceCompiled(m_handlerSourceCompiled);

        // initialized status window and enabled setting for buttons
        OnInterpreterStatusChanged(*m_interpreter); 
    }

    void InterpreterWidget::OnButtonStartPressed()
    {
        m_interpreter->Execute();
    }

    void InterpreterWidget::OnButtonStopPressed()
    {
        m_interpreter->Stop();
    }

    void InterpreterWidget::OnInterpreterStatusChanged(const Interpreter& interpreter)
    {
        using namespace InterpreterWidgetCpp;

        const auto status = interpreter.GetStatus();
        const auto startStopButtonEnabled = ToStartStopButtonEnabled(status);
        m_view->buttonStart->setEnabled(startStopButtonEnabled.first);
        m_view->buttonStop->setEnabled(startStopButtonEnabled.second);

        const auto statusString = interpreter.GetStatusString();
        m_view->interpreterStatus->setText(AZStd::string::format("%.*s", AZ_STRING_ARG(statusString)).c_str());
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
}

