/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#if !defined(Q_MOC_RUN)
#include <QMenu>

#include "EditorCommon.h"
#endif

class HierarchyWidget;

class HierarchyMenu
    : public QMenu
{
    Q_OBJECT

public:

    enum Show
    {
        kNone                           = 0x0000,

        kCutCopyPaste                   = 0x0001,
        kSavePrefab                     = 0x0002,
        kNew_EmptyElement               = 0x0004,
        kNew_EmptyElementAtRoot         = 0x0008,
        kNew_ElementFromPrefabs         = 0x0010,
        kNew_ElementFromPrefabsAtRoot   = 0x0020,
        kAddComponents                  = 0x0040,
        kDeleteElement                  = 0x0080,
        kNewSlice                       = 0x0100,
        kNew_InstantiateSlice           = 0x0200,
        kNew_InstantiateSliceAtRoot     = 0x0400,
        kPushToSlice                    = 0x0800,
        kEditorOnly                     = 0x1000,
        kFindElements                   = 0x2000,
        kAll                            = 0xffff
    };

    HierarchyMenu(HierarchyWidget* hierarchy,
        size_t showMask,
        bool addMenuForNewElement,
        const QPoint* optionalPos = nullptr);

private:

    void CutCopyPaste(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems);

    void SavePrefab(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems);

    void SliceMenuItems(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        size_t showMask);

    void New_EmptyElement(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QMenu* menu,
        bool addAtRoot,
        const QPoint* optionalPos);
    void New_ElementFromPrefabs(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QMenu* menu,
        bool addAtRoot,
        const QPoint* optionalPos);
    void New_ElementFromSlice(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QMenu* menu,
        bool addAtRoot,
        const QPoint* optionalPos);

    void AddComponents(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems);

    void DeleteElement(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems);

    void FindElements(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems);

    void EditorOnly(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems);
};
