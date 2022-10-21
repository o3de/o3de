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

namespace ScriptCanvas
{
    struct NodeReplacementConfiguration;
}

namespace ScriptCanvasEditor
{
    using NodeReplacementId = AZStd::string;

    class NodeReplacementRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ScriptCanvas::NodeReplacementConfiguration GetNodeReplacementConfiguration(const NodeReplacementId& replacementId) const = 0;
    };

    using NodeReplacementRequestBus = AZ::EBus<NodeReplacementRequests>;
} // namespace ScriptCanvasEditor
