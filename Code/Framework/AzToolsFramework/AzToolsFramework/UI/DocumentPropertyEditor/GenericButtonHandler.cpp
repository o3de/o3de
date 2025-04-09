/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/GenericButtonHandler.h>

namespace AzToolsFramework
{
    GenericButtonHandler::GenericButtonHandler()
    {
        setAutoRaise(true);

        connect(this, &QToolButton::clicked, this, &GenericButtonHandler::OnClicked);
    }

    void GenericButtonHandler::SetValueFromDom(const AZ::Dom::Value& node)
    {
        using AZ::DocumentPropertyEditor::Nodes::GenericButton;
        // Cache the node so we can query OnActivate on it when we're pressed.
        m_node = node;
        auto buttonText = GenericButton::ButtonText.ExtractFromDomNode(node).value_or("");
        setText(QString::fromUtf8(AZStd::string(buttonText).c_str()));
    }

    void GenericButtonHandler::OnClicked()
    {
        using AZ::DocumentPropertyEditor::Nodes::GenericButton;
        GenericButton::OnActivate.InvokeOnDomNode(m_node);
    }
} // namespace AzToolsFramework
