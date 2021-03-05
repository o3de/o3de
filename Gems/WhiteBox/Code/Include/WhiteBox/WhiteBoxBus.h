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
