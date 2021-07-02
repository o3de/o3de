/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <Vegetation/Ebuses/AreaConfigRequestBus.h>

namespace Vegetation
{
    class AreaBlenderRequests
        : public AreaConfigRequests
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool GetInheritBehavior() const = 0;
        virtual void SetInheritBehavior(bool value) = 0;

        virtual bool GetPropagateBehavior() const = 0;
        virtual void SetPropagateBehavior(bool value) = 0;

        virtual size_t GetNumAreas() const = 0;
        virtual AZ::EntityId GetAreaEntityId(int index) const = 0;
        virtual void RemoveAreaEntityId(int index) = 0;
        virtual void AddAreaEntityId(AZ::EntityId entityId) = 0;
    };

    using AreaBlenderRequestBus = AZ::EBus<AreaBlenderRequests>;
}
