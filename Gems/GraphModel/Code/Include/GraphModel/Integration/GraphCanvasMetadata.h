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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/map.h>

// Graph Model
#include <GraphModel/Model/Common.h>

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
        
        virtual ~GraphCanvasMetadata() = default;

        static void Reflect(AZ::ReflectContext* reflectContext);

    private:
        friend class GraphController;

        // I tried using a unique_ptr but SerializeContext didn't like it
        typedef AZStd::shared_ptr<GraphCanvas::EntitySaveDataContainer> EntitySaveDataContainerPtr;

        // Using a map instead of unordered_map for simpler xml diffs
        typedef AZStd::map<GraphModel::NodeId, EntitySaveDataContainerPtr> NodeMetadataMap;
        typedef AZStd::map<AZ::EntityId, EntitySaveDataContainerPtr> OtherMetadataMap;
        
        //! Graph Canvas metadata that pertains to the entire scene
        EntitySaveDataContainerPtr m_sceneMetadata;

        //! Graph Canvas metadata that pertains to each node in our data model. For example,
        //! the position of each node.
        NodeMetadataMap m_nodeMetadata;

        //! Graph Canvas metadata that is not related to our data model. For example,
        //! Comment nodes and Group Box nodes.
        OtherMetadataMap m_otherMetadata;
    };
}
