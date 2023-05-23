/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/ContainerActionButtonHandler.h>

namespace AzToolsFramework
{
    ContainerActionButtonHandler::ContainerActionButtonHandler()
    {
    }

    void ContainerActionButtonHandler::SetValueFromDom(const AZ::Dom::Value& node)
    {
        GenericButtonHandler::SetValueFromDom(node);

        static QIcon s_iconAdd(QStringLiteral(":/stylesheet/img/UI20/add-16.svg"));
        static QIcon s_iconRemove(QStringLiteral(":/stylesheet/img/UI20/delete-16.svg"));
        static QIcon s_iconClear(QStringLiteral(":/stylesheet/img/UI20/delete-16.svg"));

        using AZ::DocumentPropertyEditor::Nodes::ContainerAction;
        using AZ::DocumentPropertyEditor::Nodes::ContainerActionButton;

        ContainerAction action = ContainerActionButton::Action.ExtractFromDomNode(node).value_or(ContainerAction::AddElement);
        switch (action)
        {
        case ContainerAction::AddElement:
            setIcon(s_iconAdd);
            setToolTip("Add new child element");
            break;
        case ContainerAction::RemoveElement:
            setIcon(s_iconRemove);
            setToolTip(tr("Remove this element"));
            break;
        case ContainerAction::Clear:
            setIcon(s_iconClear);
            setToolTip(tr("Remove all elements"));
            break;
        }
    }
} // namespace AzToolsFramework
