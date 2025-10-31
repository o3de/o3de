/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Transform.h>

namespace AZ::RPI
{

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen to changes in XR Poses.
    //! For example, each Joystick is represented by an XR Pose, just as the Head orientation and location
    //! within the local or stage spaces.
    //! In particular, OpenXR systems have the concept of predicted display time for the current frame.
    //! The predicted display time is used to "locate" XR Spaces and calculate their Poses (aka transforms)
    //! as they are expected to be when the current frame is displayed. This predicted display time is calculated
    //! by each OpenXRVk::Device each frame, and this is the ideal moment to update Camera and Controller
    //! pose locations.
    class XRSpaceNotifications : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: Notifications are addressed to a single address (aka Broadcasted to whoever cares)
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: Notifications can be handled by multiple listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual ~XRSpaceNotifications() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified each frame of movements on the VR Headset.
        //! For convenience the function provides Three transforms:
        //! @param baseSpaceToHeadTm Transform that defines the Orientation and location of the user's head
        //!                          relative to the base XR Space.
        //! @param headToLeftEyeTm Transform that defines the Orientation and location of the user's Left Eye
        //!                        relative @baseSpaceToHeadTm.
        //! @param headToRightEyeTm Transform that defines the Orientation and location of the user's Right Eye
        //!                         relative @baseSpaceToHeadTm.
        //! REMARK: Upon getting this event, the application can query the XRSystem for the Poses for each Joystick.
        //! Tips: The location of the Left eye relative to the base XR Space would be:
        //!     baseSpaceToHeadTm *  headToLeftEyeTm.
        //! Equivalently for the right eye:
        //!     baseSpaceToHeadTm *  headToRightEyeTm.
        virtual void OnXRSpaceLocationsChanged(
            const AZ::Transform& baseSpaceToHeadTm,
            const AZ::Transform& headToLeftEyeTm,
            const AZ::Transform& headToRightEyeTm) = 0;

    };
    using XRSpaceNotificationBus = AZ::EBus<XRSpaceNotifications>;

} // namespace AZ::RPI
