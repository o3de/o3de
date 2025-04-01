/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzFramework/PaintBrush/PaintBrushSettings.h>

namespace AzFramework
{
    //! EBus that supports the painting API at runtime.
    //! This is exposed to the Behavior Context and provides a common runtime interface for any paintable component.
    class PaintBrushSession : public AZ::EBusTraits
    {
    public:
        // Overrides the default AZ::EBusTraits handler policy to allow only one listener.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        static void Reflect(AZ::ReflectContext* context);

        //! Start an image modification session.
        //! This will create a modification buffer that contains an uncompressed copy of the current image data.
        //! @param paintableEntityId The entity to connect with a paintbrush session.
        virtual AZ::Uuid StartPaintSession(const AZ::EntityId& paintableEntityId) = 0;

        //! Finish an image modification session.
        //! Clean up any helper structures used during image modification.
        virtual void EndPaintSession(const AZ::Uuid& sessionId) = 0;

        //! The following APIs are the high-level image modification APIs.
        //! These enable image modifications through paintbrush controls.

        //! Start a brush stroke with the given brush settings for color/intensity/opacity.
        //! @param brushSettings The paintbrush settings containing the color/intensity/opacity to use for the brush stroke.
        virtual void BeginBrushStroke(const AZ::Uuid& sessionId, const AzFramework::PaintBrushSettings& brushSettings) = 0;

        //! End the current brush stroke.
        virtual void EndBrushStroke(const AZ::Uuid& sessionId) = 0;

        //! Returns whether or not the brush is currently in a brush stroke.
        //! @return True if a brush stroke has been started, false if not.
        virtual bool IsInBrushStroke(const AZ::Uuid& sessionId) const = 0;

        //! Reset the brush tracking so that the next action will be considered the start of a stroke movement instead of a continuation.
        //! This is useful for handling discontinuous movement (like moving off the edge of the surface on one side and back on from
        //! a different side).
        virtual void ResetBrushStrokeTracking(const AZ::Uuid& sessionId) = 0;

        //! Apply a paint color to the underlying data based on brush movement and settings.
        //! @param brushCenter The current center of the paintbrush.
        //! @param brushSettings The current paintbrush settings.
        virtual void PaintToLocation(
            const AZ::Uuid& sessionId, const AZ::Vector3& brushCenter, const AzFramework::PaintBrushSettings& brushSettings) = 0;

        //! Smooth the underlying data based on brush movement and settings.
        //! @param brushCenter The current center of the paintbrush.
        //! @param brushSettings The current paintbrush settings.
        virtual void SmoothToLocation(
            const AZ::Uuid& sessionId, const AZ::Vector3& brushCenter, const AzFramework::PaintBrushSettings& brushSettings) = 0;
    };

    using PaintBrushSessionBus = AZ::EBus<PaintBrushSession>;

} // namespace AzFramework
