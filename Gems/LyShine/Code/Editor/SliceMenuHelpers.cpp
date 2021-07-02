/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"
#include "AssetTreeEntry.h"

namespace SliceMenuHelpers
{

    void CreateMenuActionsAndSubMenus(
        AssetTreeEntry* sliceAssetTree,
        HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QMenu* parentMenu,
        bool addAtRoot,
        const AZ::Vector2& viewportPosition)
    {
        // Add the file entries at this level alphabetically
        for (auto fileEntry : sliceAssetTree->m_files)
        {
            QAction* action = new QAction(fileEntry.first.c_str(), parentMenu);
            parentMenu->addAction(action);

            // get the pathname relative to the asset root
            const AZ::Data::AssetId& assetId = fileEntry.second;

            // Connect the action to a lamba that instantiates the slice
            QObject::connect(action,
                &QAction::triggered,
                hierarchy,
                [assetId, hierarchy, addAtRoot, viewportPosition]([[maybe_unused]] bool checked)
                {
                    if (addAtRoot)
                    {
                        hierarchy->clearSelection();
                    }

                    hierarchy->GetEditorWindow()->GetSliceManager()->InstantiateSlice(assetId, viewportPosition);
                });
        }

        // Add the sub-folder entries at this level alphabetically
        for (auto folderEntry : sliceAssetTree->m_folders)
        {
            QMenu* subMenu = parentMenu->addMenu(folderEntry.first.c_str());
            CreateMenuActionsAndSubMenus(folderEntry.second,
                hierarchy, selectedItems, subMenu, addAtRoot, viewportPosition);
        }
    }

    void CreateInstantiateSliceMenu(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QMenu* parent,
        bool addAtRoot,
        const AZ::Vector2& viewportPosition)
    {
        AssetTreeEntry* sliceAssetTree = hierarchy->GetEditorWindow()->GetSliceLibraryTree();

        QMenu* sliceLibraryMenu = parent->addMenu(QString("Element%1 from Slice &Library").arg(!addAtRoot && selectedItems.size() > 1 ? "s" : ""));

        CreateMenuActionsAndSubMenus(sliceAssetTree,
            hierarchy, selectedItems, sliceLibraryMenu, addAtRoot, viewportPosition);
    }

}   // namespace SliceMenuHelpers
