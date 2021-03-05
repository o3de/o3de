/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "MultiLineTextEditHandler.h"
#include <AzToolsFramework/Debug/TraceContext.h>

namespace AzToolsFramework
{
    QWidget* MultiLineTextEditHandler::CreateGUI(QWidget* parent)
    {
        GrowTextEdit* textEdit = aznew GrowTextEdit(parent);
        connect(textEdit, &GrowTextEdit::textChanged, this, [textEdit]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, textEdit);
        });
        connect(textEdit, &GrowTextEdit::EditCompleted, this, [textEdit]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, textEdit);
        });

        return textEdit;
    }

    AZ::u32 MultiLineTextEditHandler::GetHandlerName() const
    {
        return AZ_CRC("MultiLineEdit", 0xf5d93777);
    }

    bool MultiLineTextEditHandler::AutoDelete() const
    {
        return true;
    }

    void MultiLineTextEditHandler::ConsumeAttribute(GrowTextEdit* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        AZ_TraceContext("Attribute name", debugName);

        if (attrib == AZ_CRC("PlaceholderText", 0xa23ec278))
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
        GUI->blockSignals(true);
        GUI->SetText(instance);
        GUI->blockSignals(false);
        return true;
    }

    void RegisterMultiLineEditHandler()
    {
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew MultiLineTextEditHandler());
    }
}

#include "UI/PropertyEditor/moc_MultiLineTextEditHandler.cpp"
