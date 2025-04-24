/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI/XRRenderingInterface.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ::RHI
{
    class XRRenderingInterface;
}

namespace AZ::RPI
{
    static constexpr int XRMaxNumControllers = 2;
    static constexpr int XRMaxNumViews = 2;
    class PassTemplate;
    class AttachmentImage;

    //! XR View specific Fov data (in radians).
    struct FovData
    {
        //! angleLeft is the angle of the left side of the field of view. For a symmetric field of view this value is negative.
        float m_angleLeft = 0.0f;
        //! angleRight is the angle of the right side of the field of view.
        float m_angleRight = 0.0f;
        //! angleUp is the angle of the top part of the field of view.
        float m_angleUp = 0.0f;
        //! angleDown is the angle of the bottom part of the field of view. For a symmetric field of view this value is negative.
        float m_angleDown = 0.0f;
    };

    //! XR pose specific data 
    struct PoseData
    {
        AZ::Quaternion m_orientation = AZ::Quaternion::CreateIdentity();
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
    };

    //! This class contains the interface related to XR but significant to RPI level functionality
    class ATOM_RPI_PUBLIC_API XRRenderingInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(XRRenderingInterface, AZ::SystemAllocator);
        AZ_RTTI(XRRenderingInterface, "{18177EAF-3014-4349-A28F-BF58442FFC2B}");

        XRRenderingInterface() = default;
        virtual ~XRRenderingInterface() = default;

        //! This api is used to create a XRInstance.
        virtual AZ::RHI::ResultCode InitInstance() = 0;

        //! This api is used to acquire swapchain image for the provided view index. 
        virtual void AcquireSwapChainImage(const AZ::u32 viewIndex) = 0;

        //! Return the number of views associated with the device.
        virtual AZ::u32 GetNumViews() const  = 0;

        //! Returns true if rendering data is valid for the current frame.
        virtual bool ShouldRender() const = 0;

        //! Return the swapChain width (in pixels) associated with the view index.
        virtual AZ::u32 GetSwapChainWidth(const AZ::u32 viewIndex) const = 0;

        //! Return the swapChain height (in pixels) associated with the view index.
        virtual AZ::u32 GetSwapChainHeight(const AZ::u32 viewIndex) const = 0;

        //! Return the swapChain format associated with the view index.
        virtual AZ::RHI::Format GetSwapChainFormat(const AZ::u32 viewIndex) const = 0;

        //! Return the Fov data (in radians) associated with provided view index.
        virtual AZ::RHI::ResultCode GetViewFov(const AZ::u32 viewIndex, AZ::RPI::FovData& outFovData) const = 0;

        //! Return the Pose data associated with provided view index.
        virtual RHI::ResultCode GetViewPose(const AZ::u32 viewIndex, PoseData& outPoseData) const = 0;

        //! Return the controller Pose data associated with provided hand Index.
        virtual RHI::ResultCode GetControllerPose(const AZ::u32 handIndex, PoseData& outPoseData) const = 0;

        //! Same as above, but conveniently returns a transform.
        virtual RHI::ResultCode GetControllerTransform(const AZ::u32 handIndex, AZ::Transform& outTransform) const = 0;

        //! Return the Pose data associated with front view.
        virtual RHI::ResultCode GetViewFrontPose(PoseData& outPoseData) const = 0;

        //! Return the Pose data associated with local view.
        //! This pose tracks center Local space which is world-locked origin, gravity-aligned to exclude
        //! pitch and roll, with +Y up, +X to the right, and -Z forward.
        virtual RHI::ResultCode GetViewLocalPose(PoseData& outPoseData) const = 0;

        //! Return the Pose data associated with local view translated and Rotated by 60 deg left or right based on handIndex
        virtual RHI::ResultCode GetControllerStagePose(const AZ::u32 handIndex, PoseData& outPoseData) const = 0;

        //! Return the controller scale data associated with provided hand Index.
        virtual float GetControllerScale(const AZ::u32 handIndex) const = 0;

        //! Creates an off-center projection matrix suitable for VR. Angles are in radians and distance is in meters.
        virtual AZ::Matrix4x4 CreateStereoscopicProjection(
            float angleLeft, float angleRight, float angleBottom, float angleTop, float nearDist, float farDist, bool reverseDepth) = 0;

        //! Returns the XR specific RHI rendering interface.
        virtual AZ::RHI::XRRenderingInterface* GetRHIXRRenderingInterface() = 0;

        //! Return the X button state from the controller.
        virtual float GetXButtonState() const = 0;

        //! Return the Y button state from the controller.
        virtual float GetYButtonState() const = 0;

        //! Return the A button state from the controller.
        virtual float GetAButtonState() const = 0;

        //! Return the B button state from the controller.
        virtual float GetBButtonState() const = 0;

        //! Return the controller related joystick state for x-axis.
        virtual float GetXJoyStickState(const AZ::u32 handIndex) const = 0;

        //! Return the controller related joystick state for y-axis.
        virtual float GetYJoyStickState(const AZ::u32 handIndex) const = 0;

        //! Return the X button state from the controller.
        virtual float GetSqueezeState(const AZ::u32 handIndex) const = 0;

        //! Return the X button state from the controller.
        virtual float GetTriggerState(const AZ::u32 handIndex) const = 0;
    };

    //! This class contains the interface that will be used to register the XR system with RPI and RHI.
    class ATOM_RPI_PUBLIC_API IXRRegisterInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(IXRRegisterInterface, AZ::SystemAllocator);
        AZ_RTTI(IXRRegisterInterface, "{89FA72F6-EA61-43AA-B129-7DC63959D5EA}");

        IXRRegisterInterface() = default;
        virtual ~IXRRegisterInterface() = default;

        //! Register XR system with RPI and RHI.
        virtual void RegisterXRInterface(XRRenderingInterface*) = 0;

        //! UnRegister XR system with RPI and RHI.
        virtual void UnRegisterXRInterface() = 0;
    };
    using XRRegisterInterface = AZ::Interface<IXRRegisterInterface>;

}
