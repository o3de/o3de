/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/Memory/SystemAllocator.h>

// Graph Model
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/Graph.h>

namespace GraphModel
{
    //!!! Start in Graph.h for high level GraphModel documentation !!!

    //! The common base class for every element in a Graph, like Node, Slot, and Connection.
    class GraphElement
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphElement, AZ::SystemAllocator);
        AZ_RTTI(GraphElement, "{FD83C7CA-556B-49F1-BACE-6E9C7A4D6347}");
        static void Reflect(AZ::ReflectContext* context);

        GraphElement() = default; // Needed by SerializeContext
        virtual ~GraphElement() = default;

        GraphElement(GraphPtr graph);

        //! Returns the Graph that owns this GraphElement
        GraphPtr GetGraph() const;

        //! Returns the GraphContext for this GraphElement
        GraphContextPtr GetGraphContext() const;

    protected:
        // Every GraphElement will at least need a pointer to the Graph, so it can convert IDs into actual element pointers.
        AZStd::weak_ptr<Graph> m_graph;
    };

} // namespace GraphModel
