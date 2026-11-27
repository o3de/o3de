/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Transform.h>

#include <Atom/RHI.Reflect/Handle.h>

namespace OpenXRVk
{
    //! In addition to reading the current Pose of a particular Action
    //! it is also possible to read both the linear and angular Velocity
    //! of an action. When calling IOpenXRActions::GetActionStatePoseWithVelocities()
    //! this is the returned data.
    struct PoseWithVelocities final
    {
        AZ_TYPE_INFO(PoseWithVelocities, "{AF5B9FF7-FB02-4DA4-89FB-66E605F728E2}");
        static void Reflect(AZ::ReflectContext* context);

        //! The current pose of the Action Space relative to the Reference Space
        //! defined by IOpenXRActions::SetBaseReferenceSpaceForPoseActions().
        //! If IOpenXRActions::SetBaseReferenceSpaceForPoseActions() is never called
        //! then this transform will be relative to the "View" Reference Space.
        AZ::Transform m_pose;

        //! To know more about how to interpret Linear and Angular Velocity see:
        //! https://registry.khronos.org/OpenXR/specs/1.0/man/html/XrSpaceVelocity.html  
        AZ::Vector3 m_linearVelocity;
        AZ::Vector3 m_angularVelocity;
    };

    //! Interface used to query the state of actions and also
    //! to drive the state of haptic feedback actions.
    //! The implementation is encouraged to expose each method
    //! of this interface as global functions in the behavior context.
    //! REMARK: This class must be instantiated AFTER OpenXRReferenceSpacesInterface.
    class IOpenXRActions
    {
    public:
        AZ_RTTI(IOpenXRActions, "{7B163790-BDBE-4C5B-832E-768CF5CDF585}");
        AZ_DISABLE_COPY_MOVE(IOpenXRActions);

        IOpenXRActions() = default;
        virtual ~IOpenXRActions() = default;

        //! An opaque handle that will provide efficient
        //! access to an Action State data. 
        using ActionHandle = AZ::RHI::Handle<uint16_t, PoseWithVelocities>; // IOpenXRActions > ;

        //! Returns a list with the names of all Action Sets that are attached to the
        //! current active Session.
        virtual AZStd::vector<AZStd::string> GetAllActionSets() const = 0;

        //! @returns:
        //!     If successful:
        //!     - true means the action set exists and is active.
        //!     - false means the action set exists and is inactive.
        virtual AZ::Outcome<bool, AZStd::string> GetActionSetState(const AZStd::string& actionSetName) const = 0;

        //! @returns:
        //!     if successful:
        //!     returns the current state of the action set before changing it.
        virtual AZ::Outcome<bool, AZStd::string> SetActionSetState(const AZStd::string& actionSetName, bool activate) = 0;

        //! @returns If an action with the given name exists, returns a successful outcome that contains
        //!     a handle that the caller will further utilize when calling any of the GetActionState*** methods. 
        virtual ActionHandle GetActionHandle(const AZStd::string& actionSetName, const AZStd::string& actionName) const = 0;

        //! @returns A successful outcome if the action is ACTIVE and representable by the given data type. In particular,
        //!     if an action is of type Vector2, and it is attempted to be read with GetActionStateBoolean() then
        //!     most likely the returned outcome will be an error.
        //! @remark When the user puts the controller/joystick down, the controllers will be automatically deactivated
        //!     and the Outcome won't be successful anymore. 
        virtual AZ::Outcome<bool, AZStd::string> GetActionStateBoolean(ActionHandle actionHandle) const = 0;
        virtual AZ::Outcome<float, AZStd::string> GetActionStateFloat(ActionHandle actionHandle) const = 0;
        virtual AZ::Outcome<AZ::Vector2, AZStd::string> GetActionStateVector2(ActionHandle actionHandle) const = 0;

        //! Pose actions are also known as Action Spaces, and their Pose/Transform is always calculated
        //! using a reference/base visualized space. This is a stateful API that can be called at anytime.
        //! See OpenXRReferenceSpacesInterface to get details about Reference Spaces.
        //! By default the base Reference Space is the "View" space, which represents the user's head centroid.
        virtual AZ::Outcome<bool, AZStd::string> SetBaseReferenceSpaceForPoseActions(const AZStd::string& visualizedSpaceName) = 0;
        virtual const AZStd::string& GetBaseReferenceSpaceForPoseActions() const = 0;
        
        //! The returned transform is relative to the base Reference Space.
        virtual AZ::Outcome<AZ::Transform, AZStd::string> GetActionStatePose(ActionHandle actionHandle) const = 0;
        
        //! Same as above, but also queries Linear and Angular velocities.
        virtual AZ::Outcome<PoseWithVelocities, AZStd::string> GetActionStatePoseWithVelocities(ActionHandle actionHandle) const = 0;
        
        //! @param amplitude Will be clamped between 0.0f and 1.0f.
        virtual AZ::Outcome<bool, AZStd::string> ApplyHapticVibrationAction(ActionHandle actionHandle, uint64_t durationNanos, float frequencyHz, float amplitude) = 0;
        virtual AZ::Outcome<bool, AZStd::string> StopHapticVibrationAction(ActionHandle actionHandle) = 0;
    };

    using OpenXRActionsInterface = AZ::Interface<IOpenXRActions>;

} // namespace OpenXRVk