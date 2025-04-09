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
    // If only the Path or the Id is valid, attempts to fill in the missing piece.
    // If both Path and Id is valid, including after correction, returns the handle including source Data,
    // otherwise, returns null
    AZStd::optional<SourceHandle> CompleteDescription(const SourceHandle& source);
    // if CompleteDescription() succeeds, sets the handle to the result, else does nothing
    bool CompleteDescriptionInPlace(SourceHandle& source);
    // accepts any sort of path and returns one with a relative path if possible
    AZStd::optional<SourceHandle> CreateFromAnyPath(const SourceHandle& source, const AZ::IO::Path& path);

    class EditorGraph;
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

        AZ_CLASS_ALLOCATOR(GraphStatisticsHelper, AZ::SystemAllocator);
        AZ_RTTI(GraphStatisticsHelper, "{7D5B7A65-F749-493E-BA5C-6B8724791F03}");

        virtual ~GraphStatisticsHelper() = default;

        void PopulateStatisticData(const EditorGraph* editorGraph);
        
        AZStd::unordered_map< ScriptCanvas::NodeTypeIdentifier, int > m_nodeIdentifierCount;

    private:

        void RegisterNodeType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier);
    };        
}
