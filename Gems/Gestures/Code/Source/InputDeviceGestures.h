/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Devices/InputDevice.h>

#include "InputChannelGesture.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Implementation for an input device listens for mouse and touch input, then interprets it as
    //! common gestures such as multi-click/tap, drag, hold, pinch, rotate, and swipe.
    class InputDeviceGestures : public AzFramework::InputDevice
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The id used to identify the primary gestures input device
        static const AzFramework::InputDeviceId Id;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input device id identifies the gestures device (regardless of index)
        //! \param[in] inputDeviceId The input device id to check
        //! \return True if the input device id identifies the gestures device, false otherwise
        static bool IsGesturesDevice(const AzFramework::InputDeviceId& inputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify standard gesture input
        struct Gesture
        {
            static const AzFramework::InputChannelId DoublePress;
            static const AzFramework::InputChannelId Drag;
            static const AzFramework::InputChannelId Hold;
            static const AzFramework::InputChannelId Pinch;
            static const AzFramework::InputChannelId Rotate;
            static const AzFramework::InputChannelId Swipe;

            //!< All gesture channel ids
            static const AZStd::array<AzFramework::InputChannelId, 6> All;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceGestures, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputDeviceGestures, "{4E6EB405-9C01-418E-96F2-0E99A2B61C45}", InputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for verbose container class
        using ConfigsByNameMap = AZStd::unordered_map<AZStd::string, InputChannelGesture::Type*>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] gestureConfigsByName Map of gesture name/config pairs used to create channels
        explicit InputDeviceGestures(const ConfigsByNameMap& gestureConfigsByName);

        InputDeviceGestures(const InputDeviceGestures&) = delete;
    public:

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceGestures() override;

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

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        InputChannelByIdMap m_allChannelsById; //!< All gesture input channels by id
    };
} // namespace Gestures
