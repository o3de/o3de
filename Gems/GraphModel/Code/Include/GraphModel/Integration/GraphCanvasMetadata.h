/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/map.h>

// Graph Canvas
#include <GraphCanvas/Types/EntitySaveData.h>

// Graph Model
#include <GraphModel/Model/Common.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphCanvas
{
    class EntitySaveDataContainer;
}

namespace GraphModelIntegration
{
    //! This class provides a way to bundle metadata from Graph Canvas for storage
    //! in a GraphModel::Graph. The Graph class has a single AZStd::any for storing
    //! UI-specific metadata, where the node canvas stores one of these GraphCanvasMetadata.
    //! This allows the Graph's file on disk to include information about where nodes
    //! are located in the scene, bookmarks, comment blocks, node groupings, etc.
    class GraphCanvasMetadata
    {
    public:
        AZ_RTTI(GraphCanvasMetadata, "{BD95C3EB-CD09-4F82-9724-032BD1827B95}");
        AZ_CLASS_ALLOCATOR(GraphCanvasMetadata, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        GraphCanvasMetadata() = default;
        virtual ~GraphCanvasMetadata() = default;

    private:
        friend class GraphController;

        // I tried using a unique_ptr but SerializeContext didn't like it
        using EntitySaveDataContainerPtr = AZStd::shared_ptr<GraphCanvas::EntitySaveDataContainer>;

        // Using a map instead of unordered_map for simpler xml diffs
        using NodeMetadataMap = AZStd::map<GraphModel::NodeId, EntitySaveDataContainerPtr>;

        //! Graph Canvas metadata that pertains to the entire scene
        EntitySaveDataContainerPtr m_sceneMetadata;

        //! Graph Canvas metadata that pertains to each node in our data model. For example,
        //! the position of each node.
        NodeMetadataMap m_nodeMetadata;
    };

    //! Structure used to serialize the selection state for nodes and constructs so that it can be restored when loading and undoing operations
    class GraphCanvasSelectionData : public GraphCanvas::ComponentSaveData
    {
    public:
        AZ_RTTI(GraphCanvasSelectionData, "{FC18625B-1E97-415D-9832-B222DE054680}", GraphCanvas::ComponentSaveData);
        AZ_CLASS_ALLOCATOR(GraphCanvasSelectionData, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        GraphCanvasSelectionData() = default;
        virtual ~GraphCanvasSelectionData() = default;
        bool m_selected = {};
    };

} // namespace GraphModelIntegration
