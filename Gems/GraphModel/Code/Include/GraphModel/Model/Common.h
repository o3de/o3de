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
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace GraphModel
{
    //!!! Start in Graph.h for high level GraphModel documentation !!!

    // Node ID that is unique within the context of a Graph
    using NodeId = int;

    // Slot ID that is unique within the context of a Node
    struct SlotIdData;
    using SlotId = SlotIdData;

    // An Endpoint is a specific Slot within a specific Node.
    // It's basically a Slot ID that is unique within the context of an entire Graph.
    using Endpoint = AZStd::pair<NodeId, SlotId>;

    class DataType;
    using DataTypePtr = AZStd::shared_ptr<const DataType>; //!< All pointers are const since this data is immutable anyway
    using DataTypeList = AZStd::vector<DataTypePtr>;

    class IGraphContext;
    using IGraphContextPtr = AZStd::shared_ptr<IGraphContext>;
    using ConstIGraphContextPtr = AZStd::shared_ptr<const IGraphContext>;

    class Graph;
    using GraphPtr = AZStd::shared_ptr<Graph>;
    using ConstGraphPtr = AZStd::shared_ptr<const Graph>;

    class GraphElement;
    using GraphElementPtr = AZStd::shared_ptr<GraphElement>;
    using ConstGraphElementPtr = AZStd::shared_ptr<const GraphElement>;

    class Node;
    using NodePtr = AZStd::shared_ptr<Node>;
    using ConstNodePtr = AZStd::shared_ptr<const Node>;
    using NodePtrList = AZStd::vector<NodePtr>;

    class SlotDefinition;
    using SlotDefinitionPtr = AZStd::shared_ptr<const SlotDefinition>; //!< All pointers are const since this data is immutable anyway

    class Slot;
    using SlotPtr = AZStd::shared_ptr<Slot>;
    using ConstSlotPtr = AZStd::shared_ptr<const Slot>;
    using SlotPtrList = AZStd::vector<SlotPtr>;

    class Connection;
    using ConnectionPtr = AZStd::shared_ptr<Connection>;
    using ConstConnectionPtr = AZStd::shared_ptr<const Connection>;

    class ModuleGraphManager;
    using ModuleGraphManagerPtr = AZStd::shared_ptr<ModuleGraphManager>;
    using ConstModuleGraphManagerPtr = AZStd::shared_ptr<const ModuleGraphManager>;

    static const AZ::u32 DefaultWrappedNodeLayoutOrder = -1;

} // namespace GraphModel
