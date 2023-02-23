/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>

namespace GraphCanvas
{
    class DraggableNodePaletteTreeItem
        : public IconDecoratedNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(DraggableNodePaletteTreeItem, AZ::SystemAllocator);
        AZ_RTTI(DraggableNodePaletteTreeItem, "{40D29F3E-17F5-42B5-B771-FFAD7DB3CB96}", IconDecoratedNodePaletteTreeItem);

        DraggableNodePaletteTreeItem(AZStd::string_view name, EditorId editorId);
        ~DraggableNodePaletteTreeItem() override = default;
            
    protected:
        Qt::ItemFlags OnFlags() const override;
    };
}
