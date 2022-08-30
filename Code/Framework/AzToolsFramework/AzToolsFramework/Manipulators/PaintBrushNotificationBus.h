/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/functional.h>
#include <AzCore/Component/ComponentBus.h>

namespace AzToolsFramework
{
    //! PaintBrushNotificationBus is used to send out notifications whenever anything about the paintbrush has changed.
    class PaintBrushNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityComponentIdPair;

        //! ValueLookupFn returns the set of current painted values at the requested positions.
        //! This should get called in response to receiving a PaintBrushNotificationBus::OnPaint(dirtyRegion, valueLookupFn) event
        //! to get the specific painted values at every position the listener cares about within the dirtyRegion.
        //! @points The input world space positions to query.
        //! @intensities [out] The output intensities of the paintbrush that have been painted at this position.
        //! @opacity [out] The output opacities of the paintbrush that have been painted at this position.
        //! @validFlags [out] For each position, true if this point has been painted by the paintbrush, false if not.
        using ValueLookupFn = AZStd::function<void(
            AZStd::span<const AZ::Vector3> points,
            AZStd::span<float> intensities,
            AZStd::span<float> opacities,
            AZStd::span<bool> validFlags)>;

        //! OnIntensityChanged notifies listeners that the paintbrush intensity setting has changed.
        //! @param intensity The new intensity setting for the paintbrush (0=black, 1=white).
        virtual void OnIntensityChanged([[maybe_unused]] float intensity) { }

        //! OnOpacityChanged notifies listeners that the paintbrush opacity setting has changed.
        //! @param opacity The new opacity setting for the paintbrush (0=transparent, 1=opaque).
        virtual void OnOpacityChanged([[maybe_unused]] float opacity) { }

        //! OnRadiusChanged notifies listeners that the paintbrush radius setting has changed.
        //! @param radius The new radius setting for the paintbrush, in meters.
        virtual void OnRadiusChanged([[maybe_unused]] float radius) { }

        //! OnWorldSpaceChanged notifies listeners that the paintbrush transform has changed,
        //! typically due to the brush moving around in world space.
        //! This will get called in each frame that the brush transform changes.
        //! @param brushTransform The new transform for the brush position/rotation/scale.
        virtual void OnWorldSpaceChanged([[maybe_unused]] const AZ::Transform& brushTransform) { }

        //! OnPaint notifies listeners that the paintbrush has painted in a region.
        //! This will get called in each frame that the paintbrush continues to paint and the brush has moved.
        //! Since the paintbrush doesn't know how it's being used, and the system using a paintbrush doesn't know the specifics of the
        //! paintbrush shape and pattern, this works through a back-and-forth handshake.
        //! 1. The paintbrush sends the OnPaint message with the AABB of the region that has changed and a paintbrush value callback.
        //! 2. The listener calls the paintbrush value callback for each position in the region that it cares about.
        //! 3. The paintbrush responds with the specific painted values for each of those positions based on the brush shape and settings.
        //! @param dirtyArea The AABB of the area that has been painted in.
        //! @param valueLookupFn The paintbrush value callback to use to get the intensities / opacities / valid flags for specific positions.
        virtual void OnPaint([[maybe_unused]] const AZ::Aabb& dirtyArea, [[maybe_unused]] ValueLookupFn& valueLookupFn) { }
    };

    using PaintBrushNotificationBus = AZ::EBus<PaintBrushNotifications>;
} // namespace AzToolsFramework
