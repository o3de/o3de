/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

#include <GraphCanvas/Editor/EditorTypes.h>

#include <GraphModel/Model/Graph.h>

#include <Editor/Nodes/BaseNode.h>

namespace LandscapeCanvas
{
    struct LandscapeCanvasSerialization
    {
        AZ_TYPE_INFO(LandscapeCanvasSerialization, "{263F0CE3-5F3D-4297-B2DC-0B81F30BEC3E}");

        //! Mapping of the original EntityId to the EntityId of the Entity that has been
        //! copied as part of the deserialization (paste/duplicate)
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_deserializedEntities;
    };

    //! LandscapeCanvasSerializationRequests
    //! Create/delete for handling our Graph Controllers
    class LandscapeCanvasSerializationRequests : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Get/set our serialized mappings of the Landscape Canvas Entities that correspond to
        //! GraphModel nodes that have been serialized
        virtual const LandscapeCanvasSerialization& GetSerializedMappings() = 0;
        virtual void SetDeserializedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& entities) = 0;
    };

    using LandscapeCanvasSerializationRequestBus = AZ::EBus<LandscapeCanvasSerializationRequests>;

    class LandscapeCanvasRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Update the Landscape Canvas to graph the specified entity and return the GraphId
        // of the graph that was opened (or focused if already opened)
        virtual GraphCanvas::GraphId OnGraphEntity(const AZ::EntityId& entityId) = 0;

        //! Return the node (if any) matching a particular EntityId in a specified graph
        virtual GraphModel::NodePtr GetNodeMatchingEntityInGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& entityId) = 0;

        //! Return the node (if any) matching a particular EntityId and ComponentId in a specified graph
        virtual GraphModel::NodePtr GetNodeMatchingEntityComponentInGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityComponentIdPair& entityComponentId) = 0;

        //! Return a list of all nodes matching a particular EntityId in all currently open graphs
        virtual GraphModel::NodePtrList GetAllNodesMatchingEntity(const AZ::EntityId& entityId) = 0;

        //! Return a list of all nodes matching a particular EntityId and ComponentId in all currently open graphs
        virtual GraphModel::NodePtrList GetAllNodesMatchingEntityComponent(const AZ::EntityComponentIdPair& entityComponentId) = 0;
    };
    using LandscapeCanvasRequestBus = AZ::EBus<LandscapeCanvasRequests>;

    class LandscapeCanvasNodeFactoryRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Create a node for the given type in the specified graph
        virtual BaseNode::BaseNodePtr CreateNodeForType(GraphModel::GraphPtr graph, const AZ::TypeId& typeId) = 0;

        //! Create a node using a given type name in the specified graph
        virtual GraphModel::NodePtr CreateNodeForTypeName(GraphModel::GraphPtr graph, AZStd::string_view nodeName) = 0;

        //! Get the corresponding component TypeId for a given node type
        virtual const AZ::TypeId GetComponentTypeId(const AZ::TypeId& nodeTypeId) = 0;

        //! Get the index for which the given node type was registered in
        virtual int GetNodeRegisteredIndex(const AZ::TypeId& nodeTypeId) const = 0;
    };
    using LandscapeCanvasNodeFactoryRequestBus = AZ::EBus<LandscapeCanvasNodeFactoryRequests>;
} // namespace LandscapeCanvas
