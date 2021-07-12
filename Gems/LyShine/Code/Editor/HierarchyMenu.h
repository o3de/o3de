/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        kNew_EmptyElement               = 0x0004,
        kNew_EmptyElementAtRoot         = 0x0008,
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

    void SliceMenuItems(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        size_t showMask);

    void New_EmptyElement(HierarchyWidget* hierarchy,
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
