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