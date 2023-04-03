/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "PropertyHandlerChar.h"

#include <AzCore/std/string/conversions.h>

QWidget* PropertyHandlerChar::CreateGUI(QWidget* pParent)
{
    AzToolsFramework::PropertyStringLineEditCtrl* ctrl = aznew AzToolsFramework::PropertyStringLineEditCtrl(pParent);
    ctrl->setMaxLen(1);
    QObject::connect(ctrl->GetLineEdit(), &QLineEdit::editingFinished, this, [ctrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessagesBus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, ctrl);
            AzToolsFramework::PropertyEditorGUIMessagesBus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::OnEditingFinished, ctrl);
        });

    return ctrl;
}

void PropertyHandlerChar::ConsumeAttribute(AzToolsFramework::PropertyStringLineEditCtrl* /*GUI*/, AZ::u32 /*attrib*/, AzToolsFramework::PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
{
}

void PropertyHandlerChar::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, AzToolsFramework::PropertyStringLineEditCtrl* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    AZStd::string str = GUI->value();
    wchar_t character = '\0';
    AZStd::to_wstring(&character, 1, str.c_str());
    instance = character;
}

bool PropertyHandlerChar::ReadValuesIntoGUI([[maybe_unused]] size_t index, AzToolsFramework::PropertyStringLineEditCtrl* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    GUI->blockSignals(true);
    {
        // NOTE: this assumes the uint32_t can be interpreted as a wchar_t, it seems to
        // work for cases tested but may not in general.
        wchar_t wcharString[2] = { static_cast<wchar_t>(instance), 0 };
        AZStd::string val;
        AZStd::to_string(val, wcharString);
        GUI->setValue(val);
    }
    GUI->blockSignals(false);

    return false;
}

void PropertyHandlerChar::Register()
{
    AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
        &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew PropertyHandlerChar());
}

#include <moc_PropertyHandlerChar.cpp>
