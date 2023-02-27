/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Devices/InputDevice.h>
#include <AzFramework/Input/Channels/InputChannelAnalog.h>
#include <AzFramework/Input/Channels/InputChannelAxis1D.h>
#include <AzFramework/Input/Channels/InputChannelAxis2D.h>
#include <AzFramework/Input/Channels/InputChannelDigital.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace VirtualGamepad
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Implementation for a virtual gamepad input device that is controlled using a touch screen.
    class InputDeviceVirtualGamepad : public AzFramework::InputDevice
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The id used to identify the primary virtual gamepad input device
        static const AzFramework::InputDeviceId Id;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input device id identifies a virtual gamepad (regardless of index)
        //! \param[in] inputDeviceId The input device id to check
        //! \return True if the input device id identifies a virtual gamepad, false otherwise
        static bool IsVirtualGamepadDevice(const AzFramework::InputDeviceId& inputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceVirtualGamepad, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputDeviceVirtualGamepad, "{DC4B939E-66C7-4F76-B7DF-049A3F13A1C3}", InputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] buttonNames The list of button names supported by the virtual gamepad
        //! \param[in] thumbStickNames The list of thumbstick names supported by the virtual gamepad
        explicit InputDeviceVirtualGamepad(const AZStd::unordered_set<AZStd::string>& buttonNames,
                                           const AZStd::unordered_set<AZStd::string>& thumbStickNames);

    public:

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceVirtualGamepad() override;

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

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Create a button input channel
        //! \param[in] channelName The input channel name
        void CreateButtonChannel(const AZStd::string& channelName);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Create a thumb-stick axis 1D input channel
        //! \param[in] channelName The input channel name
        void CreateThumbStickAxis1DChannel(const AZStd::string& channelName);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Create a thumb-stick axis 2D input channel
        //! \param[in] channelName The input channel name
        void CreateThumbStickAxis2DChannel(const AZStd::string& channelName);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Create a thumb-stick direction input channel
        //! \param[in] channelName The input channel name
        void CreateThumbStickDirectionChannel(const AZStd::string& channelName);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Alias for verbose container class
        using ButtonChannelByNameMap = AZStd::unordered_map<AZStd::string, AzFramework::InputChannelDigital*>;
        using ThumbStickAxis1DChannelByNameMap = AZStd::unordered_map<AZStd::string, AzFramework::InputChannelAxis1D*>;
        using ThumbStickAxis2DChannelByNameMap = AZStd::unordered_map<AZStd::string, AzFramework::InputChannelAxis2D*>;
        using ThumbStickDirectionChannelByNameMap = AZStd::unordered_map<AZStd::string, AzFramework::InputChannelAnalog*>;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        InputChannelByIdMap                 m_allChannelsById;                   //!< All virtual input channels by id
        ButtonChannelByNameMap              m_buttonChannelsByName;              //!< All virtual button channels by id
        ThumbStickAxis1DChannelByNameMap    m_thumbStickAxis1DChannelsByName;    //!< All thumb-stick axis 1D channels by id
        ThumbStickAxis2DChannelByNameMap    m_thumbStickAxis2DChannelsByName;    //!< All thumb-stick axis 2D channels by id
        ThumbStickDirectionChannelByNameMap m_thumbStickDirectionChannelsByName; //!< All thumb-stick direction channels by id
    };
} // namespace VirtualGamepad
