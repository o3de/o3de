/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class EntityId;
}

namespace ScriptCanvas
{
    class Node;
    struct NodeReplacementConfiguration;
    struct NodeUpdateReport;
    struct SlotId;
}

namespace ScriptCanvasEditor
{
    using NodeReplacementId = AZStd::string;

    //! INodeReplacementRequests
    //! ScriptCanvas Editor interfaces to support node replacement in Editor
    class INodeReplacementRequests
    {
    public:
        AZ_RTTI(INodeReplacementRequests, "{1CBE56D6-1378-44C4-826A-3AC3AF3E04E9}");

        INodeReplacementRequests() = default;
        virtual ~INodeReplacementRequests() = default;

        //! Get new node replacement configuration based on the given old node replacement id
        virtual ScriptCanvas::NodeReplacementConfiguration GetNodeReplacementConfiguration(
            const NodeReplacementId& replacementId) const = 0;

        //! Replace old node based on new node replacement configuration, and return a update report including
        //! new node object and slots remapping data
        virtual ScriptCanvas::NodeUpdateReport ReplaceNodeByReplacementConfiguration(
            const AZ::EntityId& graphId, ScriptCanvas::Node* oldNode, const ScriptCanvas::NodeReplacementConfiguration& config) = 0;
    };

    class NodeReplacementRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using NodeReplacementRequestBus = AZ::EBus<INodeReplacementRequests, NodeReplacementRequests>;
} // namespace ScriptCanvasEditor
