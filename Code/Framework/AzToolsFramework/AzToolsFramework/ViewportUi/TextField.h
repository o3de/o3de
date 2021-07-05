/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace AzToolsFramework::ViewportUi::Internal
{
    //! Data class for a string label and text edit field.
    //! E.g.: ScaleX [_____].
    class TextField
    {
    public:
        TextField(
            const AZStd::string& labelText = "", const AZStd::string& fieldText = "",
            TextFieldValidationType validationType = TextFieldValidationType::String);
        ~TextField() = default;

        void ConnectEventHandler(AZ::Event<AZStd::string>::Handler& handler);

        //! Default text for the text field. Will be cast to same type as m_validationType.
        AZStd::string m_fieldText; 
        AZStd::string m_labelText;
        TextFieldValidationType m_validationType; //<! The type of validator for this text edit.
        TextFieldId m_textFieldId;
        ViewportUiElementId m_viewportId;
        AZ::Event<AZStd::string> m_textEditedEvent;
    };
} // namespace AzToolsFramework::ViewportUi::Internal
