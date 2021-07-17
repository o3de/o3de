/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/array.h>

#include <Cry_Math.h>

#include <cstdint>

namespace AZ 
{
    namespace VR 
    {
        ///
        /// Enum to describe the stereo layout of content
        ///
        enum class StereoLayout : AZ::u32
        {
            TOP_BOTTOM = 0,     //Top is Left, Bottom is Right
            BOTTOM_TOP,         //Bottom is Left, Top is Right
            //TODO: Figure out how to support LEFT_RIGHT and RIGHT_LEFT
            //TOP_BOTTOM is preferred because of the way that scan lines are ordered
            //LEFT_RIGHT,         //Left is Left, Right is Right
            //RIGHT_LEFT,         //Right is Left, Left is Right
            UNKNOWN             //This content is either not stereo or its stereo format cannot be determined             
        };


        ///
        /// Eye-specific camera info.
        ///
        struct PerEyeCameraInfo
        {
            float fov;             ///< Field-of-view of this eye. Note that each eye may have different fields-of-view.
            float aspectRatio;     ///< Aspect-ratio of this eye. Note that each eye may have different aspect ratios.
            AZ::Vector3 eyeOffset; ///< Camera-space offset for this eye relative to the non-stereo view.

            struct AsymmetricFrustumPlane
            {
                float horizontalDistance; ///< Horizontal frustum shift relative to the non-stereo frustum.
                float verticalDistance;   ///< Vertical frustum shift relative to the non-stereo frustum.

                AsymmetricFrustumPlane()
                    : horizontalDistance(1.6f)
                    , verticalDistance(0.9f)
                {
                }
            };

            AsymmetricFrustumPlane frustumPlane;

            PerEyeCameraInfo()
                : aspectRatio(16.0f / 9.0f)
                , fov(DEG2RAD(1.5f))
                , eyeOffset(0.65f, 0.0f, 0.0f)
            {
            }
        };

        ///
        /// Types of social screens supported by the engine.
        ///
        enum class HMDSocialScreen
        {
            Off = -1,
            UndistortedLeftEye,
            UndistortedRightEye,
        };

        ///
        /// Supported tracking levels.
        ///
        enum class HMDTrackingLevel
        {
            kHead, ///< The sensor reads as if the player is standing.
            kFloor, ///< Sensor reads as if the player is seated/on the floor.
            kFixed ///< Translation information is ignored, the view appears at the HMD origin
        };

        ///
        /// Human-readable info about the connected device. This info is printed to the screen when a new device is detected.
        ///
        struct HMDDeviceInfo
        {
            AZ_TYPE_INFO(HMDDeviceInfo, "{DB83AF23-CF4E-491D-A346-F5DC834D1C74}")

            static void Reflect(AZ::ReflectContext* context);

            const char* productName;
            const char* manufacturer;

            // Rendering resolution is defined as containing just a single eye.
            unsigned int renderWidth;
            unsigned int renderHeight;

            // Field of view is defined as the total field of view of the device which includes both eyes.
            float fovH;
            float fovV;
            
            HMDDeviceInfo()
                : productName(nullptr)
                , manufacturer(nullptr)
                , renderWidth(0)
                , renderHeight(0)
                , fovH(0.0f)
                , fovV(0.0f)
            {
            }
        };

        enum HMDStatus
        {
            HMDStatus_OrientationTracked = BIT(1),
            HMDStatus_PositionTracked = BIT(2),
            HMDStatus_CameraPoseTracked = BIT(3),
            HMDStatus_PositionConnected = BIT(4),
            HMDStatus_HmdConnected = BIT(5),

            HMDStatus_IsUsable = HMDStatus_HmdConnected | HMDStatus_OrientationTracked,
            HMDStatus_ControllerValid = HMDStatus_OrientationTracked | HMDStatus_PositionConnected,
        };

        ///
        /// Single device render target created and managed by the device. The renderer should make use of this render target in order to properly display
        /// the rendered content to this HMD.
        ///
        struct HMDRenderTarget
        {
            void* deviceSwapTextureSet; ///< Device-represented texture. These textures are created and maintained by the HMD's specific SDK.
            uint32 numTextures; ///< Number of textures inside of the swap set.
            void** textures;    ///< Access to the internal device textures. This array is exactly numTextures long.

            HMDRenderTarget()
                : deviceSwapTextureSet(nullptr)
                , numTextures(0)
                , textures(nullptr)
            {
            }
        };

        enum class ControllerIndex
            : uint32_t
        {
            LeftHand = 0,
            RightHand,
            MaxNumControllers
        };

        ///
        /// A specific pose of the HMD. Every HMD device has their own way of representing their
        /// current pose in 3D space. This structure acts as a common data set between any connected
        /// device and the rest of the system.
        ///
        struct PoseState
        {
            AZ_TYPE_INFO(PoseState, "{040F18D7-1163-477B-8908-47CC35737DCE}")

            static void Reflect(AZ::ReflectContext* context);

            AZ::Quaternion orientation; ///< The current orientation of the HMD.
            AZ::Vector3 position;       ///< The current position of the HMD in local space as an offset from the centered pose.
            
            PoseState()
                : orientation(AZ::Quaternion::CreateIdentity())
                , position(AZ::Vector3::CreateZero())
            {
            }
        };

        ///
        /// Dynamics (accelerations and velocities) of the current HMD. Many HMDs have the ability to track the current movements
        /// of the VR device(s) for prediction. Note that not all devices may support velocities/accelerations.
        ///
        struct DynamicsState
        {
            AZ_TYPE_INFO(DynamicsState, "{5C5E2249-8844-4790-9F7A-88703A9C18DD}")

            static void Reflect(AZ::ReflectContext* context);

            /// Angular velocity/acceleration reported in local space.
            AZ::Vector3 angularVelocity;
            AZ::Vector3 angularAcceleration;

            /// Linear velocity/acceleration reported in local space.
            AZ::Vector3 linearVelocity;
            AZ::Vector3 linearAcceleration;
            
            DynamicsState()
                : angularVelocity(0)
                , angularAcceleration(0)
                , linearVelocity(0)
                , linearAcceleration(0)
            {
            }
        };

        ///
        /// While tracking the HMD, certain parts of the devices may go off/online. For example,
        /// a controller may be disconnected or the HMD may lose rotational tracking temporarily. This
        /// struct stores a tracked state meaning a pose as well as flags that denote what part of the pose
        /// is currently valid.
        ///
        struct TrackingState
        {
            AZ_TYPE_INFO(TrackingState, "{E9CB08E8-9996-478B-AABB-EC8CCCF3B403}")

            typedef uint32 StatusFlags;

            bool CheckStatusFlags(StatusFlags flags) const
            {
                // Multiple flags can be checked simultaneously.
                return (statusFlags & flags) == flags;
            }

            static void Reflect(AZ::ReflectContext* context);

            PoseState pose;         ///< Current pose relating to this tracked state.
            DynamicsState dynamics; ///< Current state of the physics dynamics for this device.
            StatusFlags statusFlags;   ///< Bitfield denoting current tracking status. Flags defined in the enum HMDStatus.

            TrackingState()
                : statusFlags(0)
            {
            }
        };

        ///
        /// Rectangle storing the playspace defined by the user when
        /// setting up VR device.
        ///
        struct Playspace
        {
            AZ_TYPE_INFO(Playspace, "{05934537-80AA-4ABA-AB2C-71096FA7DC74}")
            AZ_CLASS_ALLOCATOR_DECL

            static void Reflect(AZ::ReflectContext* context);

            bool isValid = false;                   ///< The playspace data is valid (calibrated).
            AZStd::array<AZ::Vector3, 4> corners;   ///< Playspace corners defined in device-local space. The center of the playspace is 0.
        };

    }//namespace VR
    AZ_TYPE_INFO_SPECIALIZE(VR::ControllerIndex, "{90D4C80E-A1CC-4DBF-A131-0082C75835E8}");
}//namespace AZ
