/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>

namespace EMStudio
{
    class AnimViewportRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Update the camera view mode.
        virtual void UpdateCameraViewMode(EMStudio::RenderOptions::CameraViewMode mode) = 0;

        //! Update the camera follow up option
        virtual void UpdateCameraFollowUp(bool followUp) = 0;

        //! Update render flags
        virtual void UpdateRenderFlags(EMotionFX::ActorRenderFlagBitset renderFlags) = 0;
    };

    using AnimViewportRequestBus = AZ::EBus<AnimViewportRequests>;
} // namespace EMStudio
