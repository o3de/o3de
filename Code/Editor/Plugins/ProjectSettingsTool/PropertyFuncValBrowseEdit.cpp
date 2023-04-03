/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PropertyFuncValBrowseEdit.h"
#include "PlatformSettings_common.h"
#include "ValidationHandler.h"
#include "ValidatorBus.h"

#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>

#include <QLineEdit>
#include <QHBoxLayout>

namespace ProjectSettingsTool
{
    PropertyFuncValBrowseEditCtrl::PropertyFuncValBrowseEditCtrl(QWidget* pParent)
        : QWidget(pParent)
        , m_validator(nullptr)
    {
        QHBoxLayout* layout = new QHBoxLayout(this);
        m_browseEdit = new AzQtComponents::BrowseEdit(this);

        layout->setSpacing(4);
        layout->setContentsMargins(1, 0, 1, 0);
        layout->addWidget(m_browseEdit);

        browseEdit()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        browseEdit()->setMinimumWidth(AzToolsFramework::PropertyQTConstant_MinimumWidth);
        browseEdit()->setFixedHeight(AzToolsFramework::PropertyQTConstant_DefaultHeight);

        browseEdit()->setFocusPolicy(Qt::StrongFocus);

        setLayout(layout);
        setFocusProxy(browseEdit());
        setFocusPolicy(browseEdit()->focusPolicy());

        m_browseEdit->setClearButtonEnabled(true);
        connect(m_browseEdit, &AzQtComponents::BrowseEdit::textChanged, this, &PropertyFuncValBrowseEditCtrl::ValueChangedByUser);
        connect(m_browseEdit, &AzQtComponents::BrowseEdit::textChanged, this, &PropertyFuncValBrowseEditCtrl::ValidateAndShowErrors);
        connect(m_browseEdit, &AzQtComponents::BrowseEdit::textChanged, this, [this]([[maybe_unused]] const QString& text)
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
            });
    }

    QString PropertyFuncValBrowseEditCtrl::GetValue() const
    {
        return m_browseEdit->text();
    }

    void PropertyFuncValBrowseEditCtrl::SetValue(const QString& value)
    {
        m_browseEdit->setText(value);
    }

    void PropertyFuncValBrowseEditCtrl::SetValueUser(const QString& value)
    {
        SetValue(value);

        emit ValueChangedByUser();
    }

    FunctorValidator* PropertyFuncValBrowseEditCtrl::GetValidator()
    {
        return m_validator;
    }

    void PropertyFuncValBrowseEditCtrl::SetValidator(FunctorValidator* validator)
    {
        m_browseEdit->lineEdit()->setValidator(validator);
        m_validator = validator;
    }

    void PropertyFuncValBrowseEditCtrl::SetValidator(FunctorValidator::FunctorType validator)
    {
        FunctorValidator* val = nullptr;
        ValidatorBus::BroadcastResult(
            val,
            &ValidatorBus::Handler::GetValidator,
            validator);

        SetValidator(val);
    }

    bool PropertyFuncValBrowseEditCtrl::ValidateAndShowErrors()
    {
        if (m_validator)
        {
            FunctorValidator::ReturnType result = m_validator->ValidateWithErrors(m_browseEdit->text());
            if (result.first == QValidator::Acceptable)
            {
                m_browseEdit->lineEdit()->setToolTip("");
                return true;
            }
            else
            {
                m_browseEdit->lineEdit()->setToolTip(result.second);
                m_browseEdit->lineEdit()->setReadOnly(false);
                return false;
            }
        }
        else
        {
            return true;
        }
    }

    void PropertyFuncValBrowseEditCtrl::ForceValidate()
    {
        // Triggers update for errors
        m_browseEdit->lineEdit()->textChanged(m_browseEdit->text());
    }

    void PropertyFuncValBrowseEditCtrl::ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == Attributes::FuncValidator)
        {
            void* validator = nullptr;
            if (attrValue->Read<void*>(validator))
            {
                if (validator != nullptr)
                {
                    SetValidator(reinterpret_cast<FunctorValidator::FunctorType>(validator));
                }
            }
        }
        else if (attrib == Attributes::ClearButton)
        {
            bool enable = false;
            if (attrValue->Read<bool>(enable))
            {
                m_browseEdit->lineEdit()->setClearButtonEnabled(enable);
            }
        }
        else if (attrib == Attributes::RemovableReadOnly)
        {
            bool readOnly = false;
            if (attrValue->Read<bool>(readOnly))
            {
                m_browseEdit->lineEdit()->setReadOnly(readOnly);
            }
        }
        else if (attrib == Attributes::ObfuscatedText)
        {
            bool obfus = false;
            if (attrValue->Read<bool>(obfus) && obfus)
            {
                m_browseEdit->lineEdit()->setEchoMode(QLineEdit::Password);
            }
        }
    }

    AzQtComponents::BrowseEdit* PropertyFuncValBrowseEditCtrl::browseEdit()
    {
        return m_browseEdit;
    }

    //  Handler  ///////////////////////////////////////////////////////////////////

    PropertyFuncValBrowseEditHandler::PropertyFuncValBrowseEditHandler(ValidationHandler* valHdlr)
        : AzToolsFramework::PropertyHandler <AZStd::string, PropertyFuncValBrowseEditCtrl>()
        , m_validationHandler(valHdlr
            )
    {}

    AZ::u32 PropertyFuncValBrowseEditHandler::GetHandlerName(void) const
    {
        return Handlers::QValidatedBrowseEdit;
    }

    QWidget* PropertyFuncValBrowseEditHandler::CreateGUI(QWidget* pParent)
    {
        PropertyFuncValBrowseEditCtrl* ctrl = aznew PropertyFuncValBrowseEditCtrl(pParent);
        m_validationHandler->AddValidatorCtrl(ctrl);
        return ctrl;
    }

    void PropertyFuncValBrowseEditHandler::ConsumeAttribute(PropertyFuncValBrowseEditCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        GUI->ConsumeAttribute(attrib, attrValue, debugName);
    }

    void PropertyFuncValBrowseEditHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, PropertyFuncValBrowseEditCtrl* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetValue().toUtf8().data();
    }

    bool PropertyFuncValBrowseEditHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, PropertyFuncValBrowseEditCtrl* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        GUI->SetValue(instance.data());
        GUI->ForceValidate();
        return true;
    }

    PropertyFuncValBrowseEditHandler* PropertyFuncValBrowseEditHandler::Register(ValidationHandler* valHdlr)
    {
        PropertyFuncValBrowseEditHandler* handler = aznew PropertyFuncValBrowseEditHandler(valHdlr);
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType,
            handler);
        return handler;
    }
} // namespace ProjectSettingsTool

#include <moc_PropertyFuncValBrowseEdit.cpp>
