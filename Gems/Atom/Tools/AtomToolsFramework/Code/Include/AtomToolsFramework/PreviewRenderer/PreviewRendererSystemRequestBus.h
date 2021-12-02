/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AtomToolsFramework
{
    //! PreviewRendererSystemRequests provides an interface for PreviewRendererSystemComponent
    class PreviewRendererSystemRequests : public AZ::EBusTraits
    {
    public:
        // Only a single handler is allowed
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    };
    using PreviewRendererSystemRequestBus = AZ::EBus<PreviewRendererSystemRequests>;
} // namespace AtomToolsFramework
