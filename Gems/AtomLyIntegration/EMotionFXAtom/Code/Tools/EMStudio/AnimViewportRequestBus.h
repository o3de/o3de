/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <Integration/Rendering/RenderFlag.h>

namespace EMStudio
{
    enum CameraViewMode
    {
        FRONT,
        BACK,
        TOP,
        BOTTOM,
        LEFT,
        RIGHT,
        DEFAULT
    };

    class AnimViewportRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Reset the camera to initial state.
        virtual void ResetCamera() = 0;

        //! Set the camera view mode.
        virtual void SetCameraViewMode(CameraViewMode mode) = 0;

        //! Toggle render option flag
        virtual void ToggleRenderFlag(EMotionFX::ActorRenderFlag flag) = 0;
    };

    using AnimViewportRequestBus = AZ::EBus<AnimViewportRequests>;
} // namespace EMStudio
