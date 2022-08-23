/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/span.h>

#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    //! PaintBrushRequestBus is used to query the paintbrush for its settings and the current painted values while painting.
    class PaintBrushRequests : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityComponentIdPair;
        using MutexType = AZStd::recursive_mutex;

        //! GetRadius returns the current paintbrush radius.
        //! @return The radius of the paintbrush
        virtual float GetRadius() const = 0;

        //! GetIntensity returns the current paintbrush intensity (0=black, 1=white).
        //! @return The intensity of the paintbrush
        virtual float GetIntensity() const = 0;

        //! GetOpacity returns the current paintbrush opacity (0=transparent, 1=opaque).
        virtual float GetOpacity() const = 0;

        //! SetRadius sets the paintbrush radius.
        //! @param radius The new radius, in meters.
        virtual void SetRadius(float radius) = 0;

        //! SetIntensity sets the paintbrush intensity.
        //! @param intensity The new intensity, in 0-1 range (0=black, 1=white).
        virtual void SetIntensity(float intensity) = 0;

        //! SetOpacity sets the paintbrush opacity.
        //! @param opacity The new opacity, in 0-1 range (0=transparent, 1=opaque).
        virtual void SetOpacity(float opacity) = 0;

    protected:
        ~PaintBrushRequests() = default;
    };

    using PaintBrushRequestBus = AZ::EBus<PaintBrushRequests>;
} // namespace AzToolsFramework
