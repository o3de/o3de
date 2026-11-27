/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <XR/XRInput.h>
#include <OpenXRVk/InputDeviceXRController.h>
#include <OpenXRVk/OpenXRVkSpace.h>
#include <OpenXRVk_Platform.h>
#include <Atom/RPI.Public/XR/XRRenderingInterface.h>

namespace OpenXRVk
{
    class Device;

    // Class that will help manage XrActionSet/XrAction
    class Input final
        : public XR::Input
    {
    public:
        AZ_CLASS_ALLOCATOR(Input, AZ::SystemAllocator);
        AZ_RTTI(Input, "{97ADD1FE-27DF-4F36-9F61-683F881F9477}", XR::Input);

        static constexpr char LogName[] = "OpenXRVkInput";

        static XR::Ptr<Input> Create();

        //! Called by the session when the predicted display time has been updated (typically
        //! the device updates the predicted display time during BeginFrame).
        //! \param[in] device The device that emitted this event.
        //! \param[in] predictedTime The predicted display time for the current frame.
        //! \param[out] xrViews Vector where each Eye Pose will be stored. Eye poses are always relative to the VIEW space.
        //!                     The VIEW pose typically represents the pose of the Head (The Head is typically centered
        //!                     between both eyes). Subscript 0 is the left eye, while subscript 1 is the right eye.                   
        //! Returns true if the number of Eye Poses matches the size of @xrViews.
        bool UpdateXrSpaceLocations(const OpenXRVk::Device& device, XrTime predictedDisplayTime, AZStd::vector<XrView>& xrViews);

        //! Sync all the actions and update controller.
        //! REMARK: XrPoses are not updated in this function. Instead, poses are updated upon UpdateXrSpaceLocations().
        //! Why? Because PollActions() is called on the main thread outside of the BeginFrame()/EndFrame() loop.
        //! This means the if Poses are updated during PollActions(), those poses would be using the predicted display time
        //! of the previous frame instead of the current frame.
        void PollActions();

        //! Initialize various actions/actions sets and add support for Oculus touch bindings
        AZ::RHI::ResultCode InitInternal() override;

        //! Create controller action spaces
        AZ::RHI::ResultCode InitializeActionSpace(XrSession xrSession);

        //! Attach action sets
        AZ::RHI::ResultCode InitializeActionSets(XrSession xrSession) const;

        //! Updates a Controller/Joystick pose.
        void LocateControllerSpace(XrTime predictedDisplayTime, XrSpace baseSpace, AZ::u32 handIndex);

        //! Update pose information for the view. 
        void LocateVisualizedSpace(XrTime predictedDisplayTime, XrSpace space, XrSpace baseSpace, OpenXRVk::SpaceType visualizedSpaceType);

        //! Return Pose data for a controller attached to a hand index
        //! By default the pose data is converted per O3DE convention: Xright, Yfront, Zup.
        //! You can read the raw XR Pose data by setting @convertToO3de to false (Not recommended, but useful for debugging).
        AZ::RHI::ResultCode GetControllerPose(AZ::u32 handIndex, AZ::RPI::PoseData& outPoseData, bool convertToO3de = true) const;

        //! Same as above but returns the pose data as an AZ::Transform. The AZ::Transform also includes the controller scale.
        AZ::RHI::ResultCode GetControllerTransform(AZ::u32 handIndex, AZ::Transform& outTransform, bool convertToO3de = true) const;

        //! Returns scale for a controller attached to a hand index
        float GetControllerScale(AZ::u32 handIndex) const;

        //! Return Pose data for a tracked space type (i.e visualizedSpaceType).
        //! By default the pose data is converted per O3DE convention: Xright, Yfront, Zup.
        //! You can read the raw XR Pose data by setting @convertToO3de to false (Not recommended, but useful for debugging).
        AZ::RHI::ResultCode GetVisualizedSpacePose(OpenXRVk::SpaceType visualizedSpaceType, AZ::RPI::PoseData& outPoseData, bool convertToO3de = true) const;

        //! Same as above but returns the pose data as an AZ::Transform
        AZ::RHI::ResultCode GetVisualizedSpaceTransform(OpenXRVk::SpaceType visualizedSpaceType, AZ::Transform& outTransform, bool convertToO3de = true) const;

        //! Get the Squeeze action
        XrAction GetSqueezeAction(AZ::u32 handIndex) const;

        //! Get the Pose action
        XrAction GetPoseAction(AZ::u32 handIndex) const;

        //! Get the Vibration action
        XrAction GetVibrationAction() const;

        //! Get the Quit action
        XrAction GetQuitAction() const;

        //! Get any button state
        bool GetButtonState(const AzFramework::InputChannelId& channelId) const;

        //! Get the X button state
        bool GetXButtonState() const;

        //! Get the Y button state
        bool GetYButtonState() const;

        //! Get the A button state
        bool GetAButtonState() const;

        //! Get the B button state
        bool GetBButtonState() const;

        //! Get the joystick state for x-axis
        float GetXJoyStickState(AZ::u32 handIndex) const;

        //! Get the joystick state for y-axis
        float GetYJoyStickState(AZ::u32 handIndex) const;

        //! Get the Squeeze action
        float GetSqueezeState(AZ::u32 handIndex) const;

        //! Get the Squeeze action
        float GetTriggerState(AZ::u32 handIndex) const;

    private:
        //! Creates an XrAction
        void CreateAction(XrAction& action, XrActionType actionType,
                          const char* actionName, const char* localizedActionName,
                          uint32_t countSubactionPathCount, const XrPath* subActionPaths) const;


        void CreateActionSet(const XrInstance& xrInstance);
        void CreateAllActions(const XrInstance& xrInstance);
        XrAction GetAction(const AzFramework::InputChannelId& channelId) const;

        //! Destroy native objects
        void ShutdownInternal() override;

        //! Returns true if the number of Eye Poses matches the size of @xrViews.
        bool LocateEyeViews(XrTime predictedDisplayTime, AZStd::vector<XrView>& xrViews);

        XrActionSet m_actionSet{ XR_NULL_HANDLE };

        XrAction m_hapticAction{};
        AZStd::vector<XrActionSuggestedBinding> m_xrActionPaths{};
        AZStd::unordered_map<AzFramework::InputChannelId, AZStd::size_t> m_xrActionIndices{};

        AZStd::array<XrPath, AZ::RPI::XRMaxNumControllers> m_handSubactionPath{};
        AZStd::array<XrSpace, AZ::RPI::XRMaxNumControllers> m_handSpace{};
        AZStd::array<float, AZ::RPI::XRMaxNumControllers> m_handScale{ { 1.0f, 1.0f } };
        AZStd::array<XrBool32, AZ::RPI::XRMaxNumControllers> m_handActive{};

        AZStd::array<XrSpaceLocation, AZ::RPI::XRMaxNumControllers> m_handSpaceLocation{};
        AZStd::array<XrSpaceLocation, SpaceType::Count> m_xrVisualizedSpaceLocations{};

        AzFramework::InputDeviceXRController m_xrController{};
        AzFramework::InputDeviceXRController::Implementation* m_xrControllerImpl{};
        bool m_wasQuitPressedLastSync{ false };
    };
}
