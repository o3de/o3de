/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PropertyFuncValLineEdit.h"

#include "PlatformSettings_common.h"
#include "ValidationHandler.h"
#include "ValidatorBus.h"

#include <QLineEdit>

namespace ProjectSettingsTool
{
    PropertyFuncValLineEditCtrl::PropertyFuncValLineEditCtrl(QWidget* pParent)
        : AzToolsFramework::PropertyStringLineEditCtrl(pParent)
        , m_validator(nullptr)
    {
        connect(m_pLineEdit, &QLineEdit::textEdited, this, &PropertyFuncValLineEditCtrl::ValueChangedByUser);
        connect(m_pLineEdit, &QLineEdit::textChanged, this, &PropertyFuncValLineEditCtrl::ValidateAndShowErrors);
        connect(m_pLineEdit, &QLineEdit::textChanged, this, [this]([[maybe_unused]] const QString& text)
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
            });
    }

    QString PropertyFuncValLineEditCtrl::GetValue() const
    {
        return m_pLineEdit->text();
    }

    void PropertyFuncValLineEditCtrl::SetValue(const QString& value)
    {
        m_pLineEdit->setText(value);
    }

    void PropertyFuncValLineEditCtrl::SetValueUser(const QString& value)
    {
        SetValue(value);

        emit ValueChangedByUser();
    }

    FunctorValidator* PropertyFuncValLineEditCtrl::GetValidator()
    {
        return m_validator;
    }

    void PropertyFuncValLineEditCtrl::SetValidator(FunctorValidator* validator)
    {
        m_pLineEdit->setValidator(validator);
        m_validator = validator;
    }

    void PropertyFuncValLineEditCtrl::SetValidator(FunctorValidator::FunctorType validator)
    {
        FunctorValidator* val = nullptr;
        ValidatorBus::BroadcastResult(
            val,
            &ValidatorBus::Handler::GetValidator,
            validator);

        SetValidator(val);
    }

    bool PropertyFuncValLineEditCtrl::ValidateAndShowErrors()
    {
        if (m_validator)
        {
            FunctorValidator::ReturnType result = m_validator->ValidateWithErrors(m_pLineEdit->text());
            if (result.first == QValidator::Acceptable)
            {
                m_pLineEdit->setToolTip("");
                return true;
            }
            else
            {
                m_pLineEdit->setToolTip(result.second);
                m_pLineEdit->setReadOnly(false);
                return false;
            }
        }
        else
        {
            return true;
        }
    }

    void PropertyFuncValLineEditCtrl::ForceValidate()
    {
        // Triggers update for errors
        m_pLineEdit->textChanged(m_pLineEdit->text());
    }

    void PropertyFuncValLineEditCtrl::ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
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
                m_pLineEdit->setClearButtonEnabled(enable);
            }
        }
        else if (attrib == Attributes::RemovableReadOnly)
        {
            bool readOnly = false;
            if (attrValue->Read<bool>(readOnly))
            {
                m_pLineEdit->setReadOnly(readOnly);
            }
        }
        else if (attrib == Attributes::ObfuscatedText)
        {
            bool obfus = false;
            if (attrValue->Read<bool>(obfus) && obfus)
            {
                m_pLineEdit->setEchoMode(QLineEdit::Password);
            }
        }
    }

    //  Handler  ///////////////////////////////////////////////////////////////////

    PropertyFuncValLineEditHandler::PropertyFuncValLineEditHandler(ValidationHandler* valHdlr)
        : AzToolsFramework::PropertyHandler <AZStd::string, PropertyFuncValLineEditCtrl>()
        , m_validationHandler(valHdlr
            )
    {}

    AZ::u32 PropertyFuncValLineEditHandler::GetHandlerName(void) const
    {
        return Handlers::QValidatedLineEdit;
    }

    QWidget* PropertyFuncValLineEditHandler::CreateGUI(QWidget* pParent)
    {
        PropertyFuncValLineEditCtrl* ctrl = aznew PropertyFuncValLineEditCtrl(pParent);
        m_validationHandler->AddValidatorCtrl(ctrl);
        return ctrl;
    }

    void PropertyFuncValLineEditHandler::ConsumeAttribute(PropertyFuncValLineEditCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        GUI->ConsumeAttribute(attrib, attrValue, debugName);
    }

    void PropertyFuncValLineEditHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, PropertyFuncValLineEditCtrl* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetValue().toUtf8().data();
    }

    bool PropertyFuncValLineEditHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, PropertyFuncValLineEditCtrl* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        GUI->SetValue(instance.data());
        GUI->ForceValidate();
        return true;
    }

    PropertyFuncValLineEditHandler* PropertyFuncValLineEditHandler::Register(ValidationHandler* valHdlr)
    {
        PropertyFuncValLineEditHandler* handler = aznew PropertyFuncValLineEditHandler(valHdlr);
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType,
            handler);
        return handler;
    }
} // namespace ProjectSettingsTool

#include <moc_PropertyFuncValLineEdit.cpp>
