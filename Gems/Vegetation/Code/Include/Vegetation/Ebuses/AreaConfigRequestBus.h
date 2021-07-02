/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Vegetation
{
    class AreaConfigRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual AZ::u32 GetAreaPriority() const = 0;
        virtual void SetAreaPriority(AZ::u32 priority) = 0;

        virtual AZ::u32 GetAreaLayer() const = 0;
        virtual void SetAreaLayer(AZ::u32 type) = 0;

        virtual AZ::u32 GetAreaProductCount() const = 0;
    };
}
