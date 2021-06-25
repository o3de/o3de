/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace PrefabHelpers
{
    QAction* CreateSavePrefabAction(HierarchyWidget* hierarchy);

    void CreateAddPrefabMenu(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QMenu* parent,
        bool addAtRoot,
        const QPoint* optionalPos);
}   // namespace PrefabHelpers
