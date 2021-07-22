/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TextField.h"

namespace AzToolsFramework::ViewportUi::Internal
{
    TextField::TextField(const AZStd::string& labelText, const AZStd::string& fieldText, TextFieldValidationType validationType)
        : m_labelText(labelText)
        , m_fieldText(fieldText)
        , m_validationType(validationType)
    {
    }

    void TextField::ConnectEventHandler(AZ::Event<AZStd::string>::Handler& handler)
    {
        handler.Connect(m_textEditedEvent);
    }
} // namespace AzToolsFramework::ViewportUi::Internal
