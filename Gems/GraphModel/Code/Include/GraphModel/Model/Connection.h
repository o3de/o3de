/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// Graph Model
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/GraphElement.h>
#include <GraphModel/Model/Slot.h>

namespace GraphModel
{
    //!!! Start in Graph.h for high level GraphModel documentation !!!

    //! Defines the connection between an Output Slot and an Input Slot.
    //! Usually a Connection instance will be created by the Graph class
    //! rather than directly.
    class Connection : public GraphElement
    {
    public:
        AZ_CLASS_ALLOCATOR(Connection, AZ::SystemAllocator);
        AZ_RTTI(Connection, "{B4301AE1-98F4-474E-B0A1-18F27EEDB059}", GraphElement);
        static void Reflect(AZ::ReflectContext* context);
        
        Connection() = default; // Needed by SerializeContext
        ~Connection() override = default;

        //! Create a Connection for a specific Graph, though this doesn't actually
        //! add it to the Graph.
        Connection(GraphPtr graph, SlotPtr sourceSlot, SlotPtr targetSlot);

        //! Initializion after the Connection has been serialized in.
        //! This must be called whenever the default constructor is used.
        //! Sets the m_graph pointer and caches pointers to other GraphElements.
        void PostLoadSetup(GraphPtr graph);

        NodePtr GetSourceNode() const;
        NodePtr GetTargetNode() const;

        SlotPtr GetSourceSlot() const;
        SlotPtr GetTargetSlot() const;

        const Endpoint& GetSourceEndpoint() const;
        const Endpoint& GetTargetEndpoint() const;

    private:
        
        AZStd::weak_ptr<Slot> m_sourceSlot;
        AZStd::weak_ptr<Slot> m_targetSlot;

        Endpoint m_sourceEndpoint;
        Endpoint m_targetEndpoint;
    };


} // namespace GraphModel
