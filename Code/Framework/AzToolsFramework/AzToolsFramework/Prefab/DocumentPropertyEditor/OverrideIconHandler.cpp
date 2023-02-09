/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/DocumentPropertyEditor/OverrideIconHandler.h>

namespace AzToolsFramework::Prefab
{
    OverrideIconHandler::OverrideIconHandler()
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &OverrideIconHandler::customContextMenuRequested, this, &OverrideIconHandler::ShowContextMenu);
    }

    void OverrideIconHandler::SetValueFromDom(const AZ::Dom::Value&)
    {
        static QIcon s_overrideIcon(QStringLiteral(":/Entity/entity_modified_as_override.svg"));

        setIcon(s_overrideIcon);
        setIconSize(QSize(8, 8));
    }

    void OverrideIconHandler::ShowContextMenu(const QPoint& position)
    {
        QMenu contextMenu;
        QAction* revertAction = contextMenu.addAction(tr("Revert Override"));

        QAction* selectedItem = contextMenu.exec(mapToGlobal(position));

        if (selectedItem == revertAction)
        {
            // This is the place where revert override code will be added later.
        }
    }
} // namespace AzToolsFramework::Prefab
