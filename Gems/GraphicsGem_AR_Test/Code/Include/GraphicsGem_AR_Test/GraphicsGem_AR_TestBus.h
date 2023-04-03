/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GraphicsGem_AR_Test/GraphicsGem_AR_TestTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace GraphicsGem_AR_Test
{
    class GraphicsGem_AR_TestRequests
    {
    public:
        AZ_RTTI(GraphicsGem_AR_TestRequests, GraphicsGem_AR_TestRequestsTypeId);
        virtual ~GraphicsGem_AR_TestRequests() = default;
        // Put your public methods here
    };

    class GraphicsGem_AR_TestBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using GraphicsGem_AR_TestRequestBus = AZ::EBus<GraphicsGem_AR_TestRequests, GraphicsGem_AR_TestBusTraits>;
    using GraphicsGem_AR_TestInterface = AZ::Interface<GraphicsGem_AR_TestRequests>;

} // namespace GraphicsGem_AR_Test
