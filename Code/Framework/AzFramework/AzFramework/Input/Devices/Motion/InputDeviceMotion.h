/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Requests/InputMotionSensorRequestBus.h>
#include <AzFramework/Input/Channels/InputChannelAxis3D.h>
#include <AzFramework/Input/Channels/InputChannelQuaternion.h>
#include <AzFramework/Input/Devices/InputDevice.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Defines a generic motion input device including the ids of all its associated input channels.
    //! Platform specific implementations are defined as private implementations so that creating an
    //! instance of this generic class will work correctly on any platform that support motion input,
    //! while providing access to the device name and associated channel ids on any platform through
    //! the 'null' implementation (primarily so that the editor can use them to setup input mappings).
    class InputDeviceMotion : public InputDevice
                            , public InputMotionSensorRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The id used to identify the primary motion input device
        static const InputDeviceId Id;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input device id identifies a motion device (regardless of index)
        //! \param[in] inputDeviceId The input device id to check
        //! \return True if the input device id identifies a motion device, false otherwise
        static bool IsMotionDevice(const InputDeviceId& inputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify different types of acceleration data. Note that
        //! not all motion devices will necessarily be able to emit all types of motion sensor data,
        //! and unlike most other input channels these ones must be explicitly enabled using either:
        //! - InputSystemComponent::m_initiallyActiveMotionChannels
        //! - InputMotionSensorRequests::SetInputChannelEnabled
        struct Acceleration
        {
            static constexpr inline InputChannelId Gravity{"motion_acceleration_gravity"};
            static constexpr inline InputChannelId Raw{"motion_acceleration_raw"};
            static constexpr inline InputChannelId User{"motion_acceleration_user"};

            //!< All acceleration input channel ids
            static constexpr inline AZStd::array All
            {
                Gravity,
                Raw,
                User
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify different types of rotation rate data. Note that
        //! not all motion devices will necessarily be able to emit all types of motion sensor data,
        //! and unlike most other input channels these ones must be explicitly enabled using either:
        //! - InputSystemComponent::m_initiallyActiveMotionChannels
        //! - InputMotionSensorRequests::SetInputChannelEnabled
        struct RotationRate
        {
            static constexpr inline InputChannelId Raw{"motion_rotation_rate_raw"};
            static constexpr inline InputChannelId Unbiased{"motion_rotation_rate_unbiased"};

            //!< All rotation rate input channel ids
            static constexpr inline AZStd::array All
            {
                Raw,
                Unbiased
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify different types of magnetic field data. Note that
        //! not all motion devices will necessarily be able to emit all types of motion sensor data,
        //! and unlike most other input channels these ones must be explicitly enabled using either:
        //! - InputSystemComponent::m_initiallyActiveMotionChannels
        //! - InputMotionSensorRequests::SetInputChannelEnabled
        struct MagneticField
        {
            static constexpr inline InputChannelId North{"motion_magnetic_field_north"};
            static constexpr inline InputChannelId Raw{"motion_magnetic_field_raw"};
            static constexpr inline InputChannelId Unbiased{"motion_magnetic_field_unbiased"};

            //!< All magnetic field input channel ids
            static constexpr inline AZStd::array All
            {
                North,
                Raw,
                Unbiased
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify different types of orientation data. Note that
        //! not all motion devices will necessarily be able to emit all types of motion sensor data,
        //! and unlike most other input channels these ones must be explicitly enabled using either:
        //! - InputSystemComponent::m_initiallyActiveMotionChannels
        //! - InputMotionSensorRequests::SetInputChannelEnabled
        struct Orientation
        {
            static constexpr inline InputChannelId Current{"motion_orientation_current"};

            //!< All orientation input channel ids
            static constexpr inline AZStd::array All
            {
                Current
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceMotion, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputDeviceMotion, "{AB8AC810-1B66-4BDA-B1D1-67DD70043650}", InputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        InputDeviceMotion();

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputDeviceMotion);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceMotion() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::GetInputChannelsById
        const InputChannelByIdMap& GetInputChannelsById() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::IsSupported
        bool IsSupported() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputMotionSensorRequests::GetInputChannelEnabled
        bool GetInputChannelEnabled(const InputChannelId& channelId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputMotionSensorRequests::SetInputChannelEnabled
        void SetInputChannelEnabled(const InputChannelId& channelId, bool enabled) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::ApplicationLifecycleEvents::OnApplicationSuspended
        void OnApplicationSuspended(Event lastEvent) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::ApplicationLifecycleEvents::OnApplicationSuspended
        void OnApplicationResumed(Event lastEvent) override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Alias for verbose container class
        using AccelerationChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAxis3D*>;
        using RotationRateChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAxis3D*>;
        using MagneticFieldChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAxis3D*>;
        using OrientationChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelQuaternion*>;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        InputChannelByIdMap         m_allChannelsById;           //!< All motion channels by id
        AccelerationChannelByIdMap  m_accelerationChannelsById;  //!< Acceleration channels by id
        RotationRateChannelByIdMap  m_rotationRateChannelsById;  //!< Rotation rate channels by id
        MagneticFieldChannelByIdMap m_magneticFieldChannelsById; //!< Magnetic field channels by id
        OrientationChannelByIdMap   m_orientationChannelsById;   //!< Orientation channels by id
        InputChannelIdSet           m_enabledMotionChannelIds;   //!< Currently enabled channels ids

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base class for platform specific implementations of motion input devices
        class Implementation
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator, 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default factory create function
            //! \param[in] inputDevice Reference to the input device being implemented
            static Implementation* Create(InputDeviceMotion& inputDevice);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] inputDevice Reference to the input device being implemented
            Implementation(InputDeviceMotion& inputDevice);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Disable copying
            AZ_DISABLE_COPY_MOVE(Implementation);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Destructor
            virtual ~Implementation();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query the connected state of the input device
            //! \return True if the input device is currently connected, false otherwise
            virtual bool IsConnected() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Tick/update the input device to broadcast all input events since the last frame
            virtual void TickInputDevice() = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Refresh the currently enabled motion sensors based on the channels that are enabled
            //! \param[in] enabledChannelIds Set of motion input channel ids that should be enabled
            virtual void RefreshMotionSensors(const InputChannelIdSet& enabledChannelIds) = 0;

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process raw motion sensor data that has been obtained during since the last frame
            //! This function is not thread safe, and so should only be called from the main thread.
            //! \param[in] channelId The input channel id
            //! \param[in] data The raw motion sensor data
            void ProcessAccelerationData(const InputChannelId& channelId, const AZ::Vector3& data);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process raw motion sensor data that has been obtained during since the last frame
            //! This function is not thread safe, and so should only be called from the main thread.
            //! \param[in] channelId The input channel id
            //! \param[in] data The raw motion sensor data
            void ProcessRotationRateData(const InputChannelId& channelId, const AZ::Vector3& data);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process raw motion sensor data that has been obtained during since the last frame
            //! This function is not thread safe, and so should only be called from the main thread.
            //! \param[in] channelId The input channel id
            //! \param[in] data The raw motion sensor data
            void ProcessMagneticFieldData(const InputChannelId& channelId, const AZ::Vector3& data);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process raw motion sensor data that has been obtained during since the last frame
            //! This function is not thread safe, and so should only be called from the main thread.
            //! \param[in] channelId The input channel id
            //! \param[in] data The raw motion sensor data
            void ProcessOrientationData(const InputChannelId& channelId, const AZ::Quaternion& data);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Reset the state of all this input device's associated input channels
            void ResetInputChannelStates();

        private:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            InputDeviceMotion& m_inputDevice; //!< Reference to the input device
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the implementation of this input device
        //! \param[in] implementation The new implementation
        void SetImplementation(AZStd::unique_ptr<Implementation> impl) { m_pimpl = AZStd::move(impl); }

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Private pointer to the platform specific implementation
        AZStd::unique_ptr<Implementation> m_pimpl;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Helper class that handles requests to create a custom implementation for this device
        InputDeviceImplementationRequestHandler<InputDeviceMotion> m_implementationRequestHandler;
    };
} // namespace AzFramework
