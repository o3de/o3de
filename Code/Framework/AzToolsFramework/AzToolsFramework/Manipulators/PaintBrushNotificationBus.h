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

        //! Returns the set of current painted values at the requested positions.
        //! This should get called in response to receiving a PaintBrushNotificationBus::OnPaint(dirtyRegion, valueLookupFn, blendFn) event
        //! to get the specific painted values at every position the listener cares about within the dirtyRegion.
        //! @points The input world space positions to query.
        //! @validPoints [out] The output positions that are valid for this paintbrush.
        //! @opacities [out] The output per-pixel opacities of the paintbrush that have been painted at this position (0-1 range).
        using ValueLookupFn = AZStd::function<void(
            AZStd::span<const AZ::Vector3> points,
            AZStd::vector<AZ::Vector3>& validPoints,
            AZStd::vector<float>& opacities)>;

        //! Returns the base value blended with the paint brush intensity / opacity based on paint brush blend mode.
        //! This should get called in response to receiving a PaintBrushNotificationBus::OnPaint(dirtyRegion, valueLookupFn, blendFn) event
        //! to blend a base value with a paintbrush value.
        //! @baseValue The input base value from whatever data is being painted (0-1).
        //! @intensity The paint brush intensity at this position (0-1).
        //! @opacity The paint brush opacity at this position (0-1).
        //! @return The blended value from 0-1.
        using BlendFn = AZStd::function<float(float baseValue, float intensity, float opacity)>;

        //! Notifies listeners that the paintbrush transform has changed,
        //! typically due to the brush moving around in world space.
        //! This will get called in each frame that the brush transform changes.
        //! @param brushTransform The new transform for the brush position/rotation/scale.
        virtual void OnWorldSpaceChanged([[maybe_unused]] const AZ::Transform& brushTransform)
        {
        }

        //! Notifies listeners that the paint mode has been entered.
        virtual void OnPaintModeBegin()
        {
        }

        //! Notifies listeners that the paint mode is exiting.
        virtual void OnPaintModeEnd()
        {
        }

        //! Notifies listeners that a paint stroke has begun.
        //! @param intensity The intensity of the paint stroke (0-1)
        //! @param opacity The opacity of the paint stroke (0-1)
        virtual void OnPaintStrokeBegin([[maybe_unused]] float intensity, [[maybe_unused]] float opacity)
        {
        }

        //! Notifies listeners that a paint stroke has ended.
        virtual void OnPaintStrokeEnd()
        {
        }

        //! Notifies listeners that the paintbrush has painted in a region.
        //! This will get called in each frame that the paintbrush continues to paint and the brush has moved.
        //! Since the paintbrush doesn't know how it's being used, and the system using a paintbrush doesn't know the specifics of the
        //! paintbrush shape and pattern, this works through a back-and-forth handshake.
        //! 1. The paintbrush sends the OnPaint message with the AABB of the region that has changed and a paintbrush value callback.
        //! 2. The listener calls the paintbrush value callback for each position in the region that it cares about.
        //! 3. The paintbrush responds with the specific painted values for each of those positions based on the brush shape and settings.
        //! 4. The listener uses the blendFn to blend values together using the paint brush blending method.
        //! @param dirtyArea The AABB of the area that has been painted in.
        //! @param valueLookupFn The paintbrush value callback to use to get the intensities / opacities / valid flags for
        //! specific positions.
        //! @param blendFn The paintbrush callback to use to blend values together.
        virtual void OnPaint(
            [[maybe_unused]] const AZ::Aabb& dirtyArea, [[maybe_unused]] ValueLookupFn& valueLookupFn, [[maybe_unused]] BlendFn& blendFn)
        {
        }
    };

    using PaintBrushNotificationBus = AZ::EBus<PaintBrushNotifications>;
} // namespace AzToolsFramework
