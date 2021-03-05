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

#include <AzCore/RTTI/RTTI.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>

namespace GraphCanvas
{
    class DraggableNodePaletteTreeItem
        : public IconDecoratedNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(DraggableNodePaletteTreeItem, AZ::SystemAllocator, 0);
        AZ_RTTI(DraggableNodePaletteTreeItem, "{40D29F3E-17F5-42B5-B771-FFAD7DB3CB96}", IconDecoratedNodePaletteTreeItem);

        DraggableNodePaletteTreeItem(AZStd::string_view name, EditorId editorId);
        ~DraggableNodePaletteTreeItem() override = default;
            
    protected:
        Qt::ItemFlags OnFlags() const override;
    };
}