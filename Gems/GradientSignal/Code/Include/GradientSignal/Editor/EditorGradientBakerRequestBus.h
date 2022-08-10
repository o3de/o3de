/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

#include <GradientSignal/Util.h>

namespace GradientSignal
{
    class GradientBakerRequests
        : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void BakeImage() = 0;

        virtual AZ::EntityId GetInputBounds() const = 0;
        virtual void SetInputBounds(const AZ::EntityId& inputBounds) = 0;
    };

    using GradientBakerRequestBus = AZ::EBus<GradientBakerRequests>;
}
