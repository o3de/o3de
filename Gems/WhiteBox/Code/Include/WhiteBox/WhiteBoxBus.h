/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace WhiteBox
{
    class RenderMeshInterface;

    //! Function object alias for creating a RenderMeshInterface.
    //! @note Used by SetRenderMeshInterfaceBuilder in WhiteBoxRequests.
    using RenderMeshInterfaceBuilderFn = AZStd::function<AZStd::unique_ptr<RenderMeshInterface>()>;

    //! White Box system level requests.
    class WhiteBoxRequests : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides ...
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Create a render mesh for use with white box data.
        virtual AZStd::unique_ptr<RenderMeshInterface> CreateRenderMeshInterface() = 0;
        //! Control what concrete implementation of RenderMeshInterface CreateRenderMeshInterface returns.
        virtual void SetRenderMeshInterfaceBuilder(RenderMeshInterfaceBuilderFn builder) = 0;

    protected:
        ~WhiteBoxRequests() = default;
    };

    using WhiteBoxRequestBus = AZ::EBus<WhiteBoxRequests>;
} // namespace WhiteBox
