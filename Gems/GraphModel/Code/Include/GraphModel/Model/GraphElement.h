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
        AZ_CLASS_ALLOCATOR(GraphElement, AZ::SystemAllocator, 0);
        AZ_RTTI(GraphElement, "{FD83C7CA-556B-49F1-BACE-6E9C7A4D6347}");

        GraphElement() = default; // Needed by SerializeContext
        virtual ~GraphElement() = default;

        GraphElement(GraphPtr graph);

        //! Returns the Graph that owns this GraphElement
        GraphPtr GetGraph() const;

        //! Returns the IGraphContext for this GraphElement
        IGraphContextPtr GetGraphContext() const;

    protected:

        AZStd::weak_ptr<Graph> m_graph; // Every GraphElement will at least need a pointer to the Graph, so it can convert IDs into actual element pointers.
    };

} // namespace GraphModel
