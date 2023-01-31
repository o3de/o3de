/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/OverrideIconHandler.h>

namespace AzToolsFramework
{
    OverrideIconHandler::OverrideIconHandler()
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        //qDebug() << connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
        qDebug() << connect(this, &OverrideIconHandler::customContextMenuRequested, this, &OverrideIconHandler::showContextMenu);
    }

    void OverrideIconHandler::SetValueFromDom(const AZ::Dom::Value& node)
    {
        static QIcon s_overrideIcon(QStringLiteral(":/Entity/entity_modified_as_override.svg"));

        // Cache the node so we can query OnActivate on it when we're pressed.
        m_node = node;

        setIcon(s_overrideIcon);
        setIconSize(QSize(8, 8));
    }

    void OverrideIconHandler::showContextMenu(const QPoint& position)
    {
        QMenu contextMenu;
        QAction* revertAction = contextMenu.addAction(tr("Revert Override"));

        QAction* selectedItem = contextMenu.exec(mapToGlobal(position));

        if (selectedItem == revertAction)
        {
            AZ_Warning("Prefab", false, "Action is clicked");
        }
    }
} // namespace AzToolsFramework
