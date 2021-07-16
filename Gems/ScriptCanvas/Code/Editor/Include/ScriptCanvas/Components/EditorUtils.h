/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/unordered_map.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>

#include <ScriptCanvas/Core/Core.h>

namespace GraphCanvas
{
    class NodePaletteTreeItem;
}

namespace ScriptCanvasEditor
{
    class Graph;
    class NodePaletteModel;

    class NodeIdentifierFactory
    {
    public:
        static ScriptCanvas::NodeTypeIdentifier ConstructNodeIdentifier(const GraphCanvas::GraphCanvasTreeItem* treeItem);
        static AZStd::vector< ScriptCanvas::NodeTypeIdentifier > ConstructNodeIdentifiers(const GraphCanvas::GraphCanvasTreeItem* treeItem);
    };
    
    class GraphStatisticsHelper
    {
    public:
        static void Reflect(AZ::ReflectContext* reflectContext);     

        AZ_CLASS_ALLOCATOR(GraphStatisticsHelper, AZ::SystemAllocator, 0);
        AZ_RTTI(GraphStatisticsHelper, "{7D5B7A65-F749-493E-BA5C-6B8724791F03}");

        virtual ~GraphStatisticsHelper() = default;

        void PopulateStatisticData(const Graph* editorGraph);
        
        AZStd::unordered_map< ScriptCanvas::NodeTypeIdentifier, int > m_nodeIdentifierCount;

    private:

        void RegisterNodeType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier);
    };        
}
