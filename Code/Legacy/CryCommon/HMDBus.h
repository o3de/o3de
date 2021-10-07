/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ISystem.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <VRCommon.h>

struct IRenderAuxGeom;

namespace AZ
{
    namespace VR
    {
        /**
        * Bus for reacting to events triggered by the VR systems
        */
        class VREvents : public AZ::EBusTraits
        {
        public:
            virtual ~VREvents() {}

            /**
            * Event triggered when an HMD initializes successfully
            */
            virtual void OnHMDInitialized() {}

            /**
            * Event triggered when an HMD shuts down
            */
            virtual void OnHMDShutdown() {}
        };

        using VREventBus = AZ::EBus<VREvents>;

        ///
        /// Device initialization bus. Each HMD device SDK should connect to this bus during startup in order to be initialized by the LY engine.
        /// Any devices that successfully initialize will be connected to the HMDDeviceBus for actual use in VR rendering.
        ///
        class HMDInitBus : public AZ::EBusTraits
        {
        public:

            virtual ~HMDInitBus() {}

            ///
            /// Attempt to initialize this device. If initialization is initially successful (device exists and is able to startup) then this device should connect to the
            /// HMDDeviceRequestBus in order to be used as an HMD from the main Open 3D Engine system.
            ///
            /// @return If true, initialization fully succeeded.
            ///
            virtual bool AttemptInit() = 0;

            ///
            /// Shutdown this device and destroy any internal context/state information that it may contain. Once this function has returned, the device should be in a
            /// totally clean state and able to re-initialized if necessary.
            ///
            virtual void Shutdown() = 0;

            ///
            /// Priority values for the HMD to set. A higher priority value means that the HMD will be be initialized before
            /// other HMDs with lower priority values.
            ///
            enum HMDInitPriority
            {
                kNullVR = -100,
                kLowest = 0,
                kMiddle = 50,
                kHighest = 100
            };

            ///
            /// Specify the initialization priority for this HMD device. Typically SDKs that have only one device that they support (e.g. Oculus) should have the highest
            /// priority so that other VR Gems don't take the device context. For example, OpenVR is capable of driving an Oculus Rift and if initialized first will control
            /// the device as opposed to the Oculus runtime.
            ///
            virtual HMDInitPriority GetInitPriority() const = 0;
        };

        using HMDInitRequestBus = AZ::EBus<HMDInitBus>;

        ///
        /// HMD device bus used to communicate with the rest of the engine. Every device supported by the engine lives in its own GEM and supports this bus. A device
        /// wraps the underlying SDK into a single object for easy use by the rest of the system. Every device created should register with the EBus in order to be picked up as
        /// a usable device during initialization via the EBus function BusConnect().
        ///
        class HMDDeviceBus
            : public AZ::EBusTraits
        {
        public:

            //////////////////////////////////////////////////////////////////////////
            // EBus Traits
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;
            using MutexType = AZStd::recursive_mutex;
            //////////////////////////////////////////////////////////////////////////

            virtual ~HMDDeviceBus() {}

            ///
            /// Simple texture descriptor to pass to the device during render target creation.
            ///
            struct TextureDesc
            {
                uint32 width;
                uint32 height;
            };

            ///
            /// Update the HMD's internal state and handle events
            /// This is NOT where tracking is updated. This is for game-time
            /// events such as controllers connecting/disconnecting or
            /// certain compositor events being triggered.
            ///
            virtual void UpdateInternalState() {}

            ///
            /// Create the render targets for a rendering device. Note that this will create all necessary render targets but the render targets will be destroyed one at a time in DestroyRenderTargets.
            ///
            /// @param renderDevice The render device to use when creating the render target.
            /// @param desc TextureDesc object denoting texture options to use during creation.
            /// @param eyeCount The number of HMDRenderTargets to be created in this function.
            /// @param renderTargets Array of pointers to HMDRenderTargets of size eyeCount created upon successful return of this function. See struct RenderTarget for more info.
            ///
            /// @returns If true, the render targets were successfully created.
            ///
            virtual bool CreateRenderTargets([[maybe_unused]] void* renderDevice, [[maybe_unused]] const TextureDesc& desc, [[maybe_unused]] size_t eyeCount, [[maybe_unused]] HMDRenderTarget* renderTargets[]) { return false; }

            ///
            /// Destroy the passed-in render target. Any device-specific texture data will be cleaned up after this function has finished executing.
            ///
            virtual void DestroyRenderTarget([[maybe_unused]] HMDRenderTarget& renderTarget) {}

            ///
            /// Take care of any frame preparations that may be necessary BEFORE rendering begins on either eye. This could be things like synchronization,
            /// clearing old state, etc.
            ///
            virtual void PrepareFrame() {}

            ///
            /// Retrieve the latest tracking state that was cached since the last call
            /// to UpdateTrackingStates.
            ///
            /// TODO: Differentiate between tracking states viable for rendering and
            /// tracking states viable for game simulation.
            ///
            virtual TrackingState* GetTrackingState() { return nullptr; }

            ///
            /// Per-eye target to submit to the device for final composition and rendering.
            ///
            struct EyeTarget
            {
                void*  renderTarget;     ///< The device render target.
                Vec2i  viewportPosition; ///< Position of the viewport pertaining to this render target.
                Vec2i  viewportSize;     ///< Size of the viewport pertaining to this render target.
            };

            ///
            /// Submit a new frame to the HMD device. Each eye should be fully rendered by this point. The device will automatically correlate the proper
            /// tracking information with this frame.
            ///
            /// @param left A reference to the left EyeTarget to present
            /// @param right A reference to the right EyeTarget to present
            ///
            virtual void SubmitFrame([[maybe_unused]] const EyeTarget& left, [[maybe_unused]] const EyeTarget& right) {}

            ///
            /// Recent the current pose for the HMD based on the current direction that the viewer is looking.
            ///
            virtual void RecenterPose() {}

            ///
            /// Set the current tracking level of the HMD. Supported tracking levels are defined in struct TrackingLevel.
            ///
            /// @param level The tracking level we want to use with this HMD
            ///
            virtual void SetTrackingLevel([[maybe_unused]] const AZ::VR::HMDTrackingLevel level) {}

            ///
            /// Write any HMD info to the console/log file(s). At a minimum this function should print the info contained in the HMDDeviceInfo object.
            ///
            virtual void OutputHMDInfo() {}

            ///
            /// Enable/disable debugging for this device. The device can decide what the most appropriate debugging information is
            /// displayed to the user (e.g. HMD position, performance info, latency timing, etc.).
            ///
            /// @param enable Set to true to enable debugging
            ///
            virtual void EnableDebugging([[maybe_unused]] bool enable) {}

            ///
            /// Draw any custom debug info for this device. This function is invoked by the HMDDebugger.
            ///
            /// @param transform Local to world-space transform.
            /// @param auxGeom A pointer to the auxiliary geometry renderer
            ///
            virtual void DrawDebugInfo([[maybe_unused]] const AZ::Transform& transform, [[maybe_unused]] IRenderAuxGeom* auxGeom) {}

            ///
            /// Get the device info object for this particular HMD. See struct HMDDeviceInfo for more details.
            ///
            /// @return A pointer to this HMD's HMDDeviceInfo struct
            ///
            virtual HMDDeviceInfo* GetDeviceInfo() { return nullptr; }

            ///
            /// Get whether or not the HMD has been initialized. The HMD has been initialized when it has fully established an interface
            /// with its necessary SDK and is ready to be used.
            ///
            /// @return True if the device has been initialized and is usable
            ///
            virtual bool IsInitialized() { return false; }

            ///
            /// Get the play space of the device, if exists
            ///
            /// @return True if the device has been initialized and is usable
            ///
            virtual const Playspace* GetPlayspace() { return nullptr; }

            ///
            /// Ask the HMD to update its internal tracking state; must be called once per frame.
            /// Must be called from the render thread (the same thread that the device submits on).
            /// This will calculate the internal tracking states fit for rendering the upcoming frame.
            ///
            virtual void UpdateTrackingStates() {}

        protected:
        };

        using HMDDeviceRequestBus = AZ::EBus<HMDDeviceBus>;

        ///
        /// Bus to define HMD debugging. This includes visualization of any HMD-specific objects as well as any
        /// VR performance metrics displayed in the HMD.
        ///
        class HMDDebuggerBus
            : public AZ::EBusTraits
        {
        public:

            virtual ~HMDDebuggerBus() {}

            ///
            /// Enable/disable the debugger.
            ///
            /// @param enable Pass in true to enable info debugging
            ///
            virtual void EnableInfo(bool enable) = 0;

            ///
            /// Enable/disable the camera debugger.
            ///
            /// @param enable Pass in true to enable camera debugging
            ///
            virtual void EnableCamera(bool enable) = 0;
        };

        using HMDDebuggerRequestBus = AZ::EBus<HMDDebuggerBus>;

    } // namespace VR
} // namespace AZ
