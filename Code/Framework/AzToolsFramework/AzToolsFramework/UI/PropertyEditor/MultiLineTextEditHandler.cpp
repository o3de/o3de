/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiLineTextEditHandler.h"
#include <AzToolsFramework/Debug/TraceContext.h>
#include <QSignalBlocker>

namespace AzToolsFramework
{
    QWidget* MultiLineTextEditHandler::CreateGUI(QWidget* parent)
    {
        GrowTextEdit* textEdit = aznew GrowTextEdit(parent);
        connect(textEdit, &GrowTextEdit::textChanged, this, [textEdit]()
        {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, textEdit);
        });
        connect(textEdit, &GrowTextEdit::EditCompleted, this, [textEdit]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, textEdit);
        });

        return textEdit;
    }

    bool MultiLineTextEditHandler::ResetGUIToDefaults(GrowTextEdit* GUI)
    {
        QSignalBlocker blocker(GUI);
        QString blankString;
        GUI->setPlaceholderText(blankString);
        GUI->setText(blankString);
        GUI->setReadOnly(false);
        return true;
    }

    AZ::u32 MultiLineTextEditHandler::GetHandlerName() const
    {
        return AZ::Edit::UIHandlers::MultiLineEdit;
    }

    bool MultiLineTextEditHandler::AutoDelete() const
    {
        return true;
    }

    void MultiLineTextEditHandler::ConsumeAttribute(GrowTextEdit* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        AZ_TraceContext("Attribute name", debugName);

        if (attrib == AZ::Edit::Attributes::PlaceholderText)
        {
            AZStd::string placeholderText;
            if (attrValue->Read<AZStd::string>(placeholderText))
            {
                GUI->setPlaceholderText(placeholderText.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, 
                    "Failed to read 'PlaceholderText' attribute from property '%s' into multi-line text field.", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::ValueText)
        {
            AZStd::string valueText;
            if (attrValue->Read<AZStd::string>(valueText))
            {
                QSignalBlocker blocker(GUI);
                GUI->SetText(valueText.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false,
                    "Failed to read 'ValueText' attribute from property '%s' into multi-line text field.", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setReadOnly(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, 
                    "Failed to read 'ReadOnly' attribute from property '%s' into multi-line text field.", debugName);
            }
        }
    }

    void MultiLineTextEditHandler::WriteGUIValuesIntoProperty(size_t /*index*/, GrowTextEdit* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
    {
        instance = GUI->GetText();
    }

    bool MultiLineTextEditHandler::ReadValuesIntoGUI(size_t /*index*/, GrowTextEdit* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
    {
        QSignalBlocker blocker(GUI);
        GUI->SetText(instance);
        return true;
    }

    void RegisterMultiLineEditHandler()
    {
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew MultiLineTextEditHandler());
    }
}

#include "UI/PropertyEditor/moc_MultiLineTextEditHandler.cpp"
