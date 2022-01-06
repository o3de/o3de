/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            class HairRequests
                : public ComponentBus
            {
            public:
                AZ_RTTI(AZ::Render::HairRequests, "{923D6B94-C6AD-4B03-B8CC-DB7E708FB9F4}");

                /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
                static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
                virtual ~HairRequests() {}

                // Add required getter and setter functions - matching the interface methods    
            };

            typedef AZ::EBus<HairRequests> HairRequestsBus;
        } // namespace Hair
    } // namespace Render
} // namespace AZ
