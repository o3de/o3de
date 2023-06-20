/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <UI/PropertyEditor/PropertyAudioCtrl.h>
#include <UI/PropertyEditor/PropertyQTConstants.h>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QToolButton>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
#include <QtWidgets/QHBoxLayout>
#include <QtGui/QMouseEvent>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>

namespace AzToolsFramework
{
    void CReflectedVarAudioControl::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CReflectedVarAudioControl>()
                ->Version(1)
                ->Field("controlName", &CReflectedVarAudioControl::m_controlName)
                ->Field("propertyType", &CReflectedVarAudioControl::m_propertyType)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CReflectedVarAudioControl>("VarAudioControl", "AudioControl")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ;
            }
        }
    }

    //=============================================================================
    // Audio Control SelectorWidget
    //=============================================================================

    AudioControlSelectorWidget::AudioControlSelectorWidget(QWidget* parent)
        : QWidget(parent)
        , m_browseEdit(nullptr)
        , m_mainLayout(nullptr)
        , m_propertyType(AudioPropertyType::NumTypes)
    {
        // create the gui
        m_mainLayout = new QHBoxLayout();
        m_mainLayout->setContentsMargins(0, 0, 0, 0);

        setFixedHeight(AzToolsFramework::PropertyQTConstant_DefaultHeight);

        m_browseEdit = new AzQtComponents::BrowseEdit(this);
        m_browseEdit->setClearButtonEnabled(true);
        m_browseEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_browseEdit->setMinimumWidth(AzToolsFramework::PropertyQTConstant_MinimumWidth);
        m_browseEdit->setFixedHeight(AzToolsFramework::PropertyQTConstant_DefaultHeight);
        m_browseEdit->setMouseTracking(true);
        m_browseEdit->setContentsMargins(0, 0, 0, 0);
        m_browseEdit->setFocusPolicy(Qt::StrongFocus);

        connect(m_browseEdit, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &AudioControlSelectorWidget::OnOpenAudioControlSelector);
        connect(m_browseEdit, &AzQtComponents::BrowseEdit::returnPressed, this,
            [this]()
            {
                SetControlName(m_browseEdit->text());
            }
        );
        QToolButton* clearButton = AzQtComponents::LineEdit::getClearButton(m_browseEdit->lineEdit());
        assert(clearButton);
        connect(clearButton, &QToolButton::clicked, this, &AudioControlSelectorWidget::OnClearControl);

        m_mainLayout->addWidget(m_browseEdit);

        setFocusProxy(m_browseEdit);
        setFocusPolicy(m_browseEdit->focusPolicy());

        setLayout(m_mainLayout);

        // todo: enable drag-n-drop from Audio Controls Editor
        //setAcceptDrops(true);
        //m_controlEdit->installEventFilter(this);
    }

    void AudioControlSelectorWidget::SetControlName(const QString& controlName)
    {
        if (m_controlName != controlName)
        {
            m_controlName = controlName;
            UpdateWidget();
            emit ControlNameChanged(m_controlName);
        }
    }

    QString AudioControlSelectorWidget::GetControlName() const
    {
        return m_controlName;
    }

    void AudioControlSelectorWidget::SetPropertyType(AudioPropertyType type)
    {
        if (m_propertyType == type)
        {
            return;
        }

        if (type != AudioPropertyType::NumTypes)
        {
            m_propertyType = type;
        }
    }

    bool AudioControlSelectorWidget::eventFilter(QObject* object, QEvent* event)
    {
        // todo: enable drag-n-drop from Audio Controls Editor
        Q_UNUSED(object);
        Q_UNUSED(event);
        return false;
    }

    void AudioControlSelectorWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        // todo: enable drag-n-drop from Audio Controls Editor
        Q_UNUSED(event);
    }

    void AudioControlSelectorWidget::dragLeaveEvent(QDragLeaveEvent* event)
    {
        // todo: enable drag-n-drop from Audio Controls Editor
        Q_UNUSED(event);
    }

    void AudioControlSelectorWidget::dropEvent(QDropEvent* event)
    {
        // todo: enable drag-n-drop from Audio Controls Editor
        Q_UNUSED(event);
    }

    void AudioControlSelectorWidget::OnClearControl()
    {
        SetControlName(QString());
        UpdateWidget();
    }

    void AudioControlSelectorWidget::OnOpenAudioControlSelector()
    {
        AZStd::string currentValue(m_controlName.toStdString().c_str());
        AZStd::string resourceResult;
        AudioControlSelectorRequestBus::EventResult(
            resourceResult, m_propertyType,
            &AudioControlSelectorRequestBus::Events::SelectResource, currentValue);
        SetControlName(QString(resourceResult.c_str()));
    }

    bool AudioControlSelectorWidget::IsCorrectMimeData(const QMimeData* data_) const
    {
        // todo: enable drag-n-drop from Audio Controls Editor
        Q_UNUSED(data_);
        return false;
    }

    void AudioControlSelectorWidget::focusInEvent(QFocusEvent* event)
    {
        m_browseEdit->lineEdit()->event(event);
        m_browseEdit->lineEdit()->selectAll();
    }

    void AudioControlSelectorWidget::UpdateWidget()
    {
        m_browseEdit->setText(m_controlName);
    }

    AZStd::string AudioControlSelectorWidget::GetResourceSelectorNameFromType(AudioPropertyType propertyType)
    {
        switch (propertyType)
        {
        case AudioPropertyType::Trigger:
            return { "AudioTrigger" };
        case AudioPropertyType::Rtpc:
            return { "AudioRTPC" };
        case AudioPropertyType::Switch:
            return { "AudioSwitch" };
        case AudioPropertyType::SwitchState:
            return { "AudioSwitchState" };
        case AudioPropertyType::Environment:
            return { "AudioEnvironment" };
        case AudioPropertyType::Preload:
            return { "AudioPreloadRequest" };
        default:
            return AZStd::string();
        }
    }

    //=============================================================================
    // Property Handler
    //=============================================================================

    QWidget* AudioControlSelectorWidgetHandler::CreateGUI(QWidget* parent)
    {
        auto newCtrl = aznew AudioControlSelectorWidget(parent);

        connect(newCtrl, &AudioControlSelectorWidget::ControlNameChanged, this,
            [newCtrl] ()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            }
        );
        return newCtrl;
    }

    void AudioControlSelectorWidgetHandler::ConsumeAttribute(widget_t* gui, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        Q_UNUSED(gui);
        Q_UNUSED(attrib);
        Q_UNUSED(attrValue);
        Q_UNUSED(debugName);
    }

    void AudioControlSelectorWidgetHandler::WriteGUIValuesIntoProperty(size_t index, widget_t* gui, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        CReflectedVarAudioControl val;
        val.m_propertyType = gui->GetPropertyType();
        val.m_controlName = gui->GetControlName().toUtf8().data();
        instance = static_cast<property_t>(val);
    }

    bool AudioControlSelectorWidgetHandler::ReadValuesIntoGUI(size_t index, widget_t* gui, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        const QSignalBlocker blocker(gui);
        CReflectedVarAudioControl val = instance;
        gui->SetPropertyType(val.m_propertyType);
        gui->SetControlName(val.m_controlName.c_str());
        return false;
    }

    void RegisterAudioPropertyHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew AudioControlSelectorWidgetHandler());
    }

} // namespace AzToolsFramework

#include "UI/PropertyEditor/moc_PropertyAudioCtrl.cpp"
