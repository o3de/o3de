/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/ContainerActionButtonHandler.h>

#include <QMessageBox>

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
        static QIcon s_iconUp(QStringLiteral(":/stylesheet/img/indicator-arrow-up.svg"));
        static QIcon s_iconDown(QStringLiteral(":/stylesheet/img/indicator-arrow-down.svg"));

        using AZ::DocumentPropertyEditor::Nodes::ContainerAction;
        using AZ::DocumentPropertyEditor::Nodes::ContainerActionButton;

        m_action = ContainerActionButton::Action.ExtractFromDomNode(node).value_or(ContainerAction::AddElement);
        switch (m_action)
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
        case ContainerAction::MoveUp:
            setIcon(s_iconUp);
            setToolTip(tr("move this element up"));
            break;
        case ContainerAction::MoveDown:
            setIcon(s_iconDown);
            setToolTip(tr("move this element down"));
            break;
        }
    }

    void ContainerActionButtonHandler::OnClicked()
    {
        using AZ::DocumentPropertyEditor::Nodes::Container;
        using AZ::DocumentPropertyEditor::Nodes::ContainerAction;

        if (m_action == ContainerAction::Clear)
        {
            auto result = QMessageBox::question(
                this, QObject::tr("Clear container?"), QObject::tr("Are you sure you want to remove all elements from this container?"));
            if (result == QMessageBox::No)
            {
                return;
            }
        }

        GenericButtonHandler::OnClicked();
    }
} // namespace AzToolsFramework
