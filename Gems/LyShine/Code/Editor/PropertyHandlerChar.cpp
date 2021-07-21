/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

#include "PropertyHandlerChar.h"

QWidget* PropertyHandlerChar::CreateGUI(QWidget* pParent)
{
    AzToolsFramework::PropertyStringLineEditCtrl* ctrl = aznew AzToolsFramework::PropertyStringLineEditCtrl(pParent);
    ctrl->setMaxLen(1);
    QObject::connect(ctrl, &AzToolsFramework::PropertyStringLineEditCtrl::valueChanged, this, [ctrl]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, ctrl);
        });

    return ctrl;
}

void PropertyHandlerChar::ConsumeAttribute(AzToolsFramework::PropertyStringLineEditCtrl* /*GUI*/, AZ::u32 /*attrib*/, AzToolsFramework::PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
{
}

void PropertyHandlerChar::WriteGUIValuesIntoProperty(size_t index, AzToolsFramework::PropertyStringLineEditCtrl* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    (int)index;
    AZStd::string str = GUI->value();
    uint32_t character = '\0';
    if (!str.empty())
    {
        Unicode::CIterator<const char*, false> pChar(str.c_str());
        character = *pChar;
    }
    instance = character;
}

bool PropertyHandlerChar::ReadValuesIntoGUI(size_t index, AzToolsFramework::PropertyStringLineEditCtrl* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    (int)index;

    GUI->blockSignals(true);
    {
        // NOTE: this assumes the uint32_t can be interpreted as a wchar_t, it seems to
        // work for cases tested but may not in general.
        wchar_t wcharString[2] = { static_cast<wchar_t>(instance), 0 };
        AZStd::string val(CryStringUtils::WStrToUTF8(wcharString));
        GUI->setValue(val);
    }
    GUI->blockSignals(false);

    return false;
}

void PropertyHandlerChar::Register()
{
    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew PropertyHandlerChar());
}

#include <moc_PropertyHandlerChar.cpp>
