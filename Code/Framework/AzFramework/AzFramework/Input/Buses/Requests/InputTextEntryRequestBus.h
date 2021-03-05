/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzFramework/Input/Devices/InputDeviceId.h>
#include <AzFramework/Input/User/LocalUserId.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to send text entry requests to connected input devices
    class InputTextEntryRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Options to specify the appearance and/or behavior of any on-screen virtual keyboard that
        //! may be displayed to the user for entering text. Depending on the specific implementation,
        //! not all of these options will be relevant, in which case they will be completely ignored.
        struct VirtualKeyboardOptions
        {
            AZStd::string m_initialText;   //!< The virtual keyboard's initial text
            AZStd::string m_titleText;     //!< The virtual keyboard's title text
            float m_normalizedMinY = 0.0f; //!< The virtual keyboard's minimum y position normalized
            LocalUserId m_localUserId = LocalUserIdAny; //!< The local user to operate the keyboard
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId using EBus<>::Event,
        //! which should be handled by only one device that has connected to the bus using that id.
        //! Input requests can also be sent using EBus<>::Broadcast, in which case they'll be sent
        //! to all input devices that have connected to the input event bus regardless of their id.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests should be handled by only one input device connected to each id
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId
        using BusIdType = InputDeviceId;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query whether text entry has already been started
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \return True if text entry has already been started, false otherwise
        virtual bool HasTextEntryStarted() const { return false; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Inform input devices that text input is expected to start (pair with StopTextInput)
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        //!
        //! \param[in] options Used to specify the appearance/behavior of any virtual keyboard shown
        virtual void TextEntryStart(const VirtualKeyboardOptions& /*options*/) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Inform input devices that text input is expected to stop (pair with StartTextInput)
        //!
        //! Called using either:
        //! - EBus<>::Broadcast (any input device can respond to the request)
        //! - EBus<>::Event(id) (the given device can respond to the request)
        virtual void TextEntryStop() {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputTextEntryRequests() = default;
    };
    using InputTextEntryRequestBus = AZ::EBus<InputTextEntryRequests>;
} // namespace AzFramework
