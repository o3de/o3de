/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/GraphScopedTypes.h>

namespace ScriptCanvas
{
    class EBusHandlerNodeRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphScopedNodeId;

        virtual void SetAddressId(const Datum& datumValue) = 0;
    };

    using EBusHandlerNodeRequestBus = AZ::EBus<EBusHandlerNodeRequests>;
}
