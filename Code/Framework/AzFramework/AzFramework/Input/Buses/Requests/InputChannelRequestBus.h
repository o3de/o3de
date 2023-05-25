/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannelId.h>

#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class InputChannel;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to query for available input channels
    class InputChannelRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputChannelId so that they are only
        //! handled by one input channel that has connected to the bus using that unique id, or they
        //! can be broadcast to all input channels that have connected to the bus, regardless of id.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests should be handled by only one input channel connected to each id
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests should be addressed to a specific channel id / device index pair.
        //! While input channel ids must be unique across different input devices, multiple devices
        //! of the same type can exist, so requests must be addressed using an id/device index pair.
        class BusIdType
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(BusIdType, AZ::SystemAllocator);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Type Info
            AZ_TYPE_INFO(BusIdType, "{FA0B740B-8917-4260-B402-05444C985AB5}");

            ////////////////////////////////////////////////////////////////////////////////////////
            // Reflection
            static void Reflect(AZ::ReflectContext* context);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] channelId Id of the input channel to address requests
            //! \param[in] deviceIndex Index of the input device to address requests
            BusIdType(const InputChannelId& channelId, AZ::u32 deviceIndex = 0)
                : m_channelId(channelId)
                , m_deviceIndex(deviceIndex)
            {}

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] channelName Name of the input channel to address requests
            //! \param[in] deviceIndex Index of the input device to address requests
            BusIdType(const char* channelName, AZ::u32 deviceIndex = 0)
                : m_channelId(channelName)
                , m_deviceIndex(deviceIndex)
            {}

            ////////////////////////////////////////////////////////////////////////////////////////
            ///@{
            //! Equality comparison operator
            //! \param[in] other Another instance of the class to compare for equality
            bool operator==(const BusIdType& other) const;
            bool operator!=(const BusIdType& other) const;
            ///@}

            ////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            InputChannelId m_channelId;   //!< Id of the input channel to address requests
            AZ::u32        m_deviceIndex; //!< Index of the input device to address requests

            //! Size_t conversion operator for std::hash to return a reasonable hash.
            [[nodiscard]] explicit constexpr operator size_t() const
            {
                size_t hashValue = m_channelId.GetNameCrc32();
                AZStd::hash_combine(hashValue, m_deviceIndex);
                return hashValue;
            }       
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Finds a specific input channel given its id and the index of the device that owns it.
        //! This convenience function wraps an EBus call to InputChannelRequests::GetInputChannel.
        //! \param[in] channelId Id of the input channel to find
        //! \param[in] deviceIndex Index of the device that owns the input channel
        //! \return Pointer to the input channel if it was found, nullptr if it was not
        static const InputChannel* FindInputChannel(const InputChannelId& channelId,
                                                    AZ::u32 deviceIndex = 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Returns the input channel uniquely identified by the id/index pair used to address calls.
        //! This should never be broadcast otherwise the channel returned will effectively be random.
        //!
        //! Examples:
        //!
        //!     // Get the left mouse button input channel
        //!     const InputChannelRequests::BusIdType requestId(InputDeviceMouse::Button::Left);
        //!     const InputChannel* inputChannel = nullptr;
        //!     InputChannelRequestBus::EventResult(inputChannel,
        //!                                         requestId,
        //!                                         &InputChannelRequests::GetInputChannel);
        //!
        //!     // Get the A button input channel for the gamepad device at index 2
        //!     const InputChannelRequests::BusIdType requestId(InputDeviceGamepad::Button::A, 2);
        //!     const InputChannel* inputChannel = nullptr;
        //!     InputChannelRequestBus::EventResult(inputChannel,
        //!                                         requestId,
        //!                                         &InputChannelRequests::GetInputChannel);
        //!
        //! \return Pointer to the input channel if it exists, nullptr otherwise
        virtual const InputChannel* GetInputChannel() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Reset the channel's state, which should broadcast an 'Ended' input notification event
        //! (if the channel is currently active) before returning the channel to the idle state.
        virtual void ResetState() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Simulate a raw input event. Please use with caution; it's designed primarily for testing
        //! purposes, and could result in strange behaviour if called while the user is interacting
        //! physically with any input device that happens to be updating the same input channel(s),
        //! or if used to simulate input of an input channel whose value is derived from the value
        //! of a different input channel (eg. the InputDeviceGamepad::ThumbStickDirection::* input
        //! channel values are derived from their respective InputDeviceGamepad::ThumbStickAxis2D).
        //!
        //! If used, it is the responsibility of the caller to reset the input channel back to it's
        //! original idle state, otherwise it may be left in a state of being permanently 'active'.
        //!
        //! \param[in] rawValue The raw input value to simulate. Analog input channels will use the
        //! value directly, digital input channels treat 0.0f as 'off' and all other values as 'on'.
        virtual void SimulateRawInput(float /*rawValue*/) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Simulate a raw input event. Please use with caution; it's designed primarily for testing
        //! purposes, and could result in strange behaviour if called while the user is interacting
        //! physically with any input device that happens to be updating the same input channel(s),
        //! or if used to simulate input of an input channel whose value is derived from the value
        //! of a different input channel (eg. the InputDeviceGamepad::ThumbStickDirection::* input
        //! channel values are derived from their respective InputDeviceGamepad::ThumbStickAxis2D).
        //!
        //! If used, it is the responsibility of the caller to reset the input channel back to it's
        //! original idle state, otherwise it may be left in a state of being permanently 'active'.
        //!
        //! \param[in] rawValueX The raw x-axis input value to simulate.
        //! \param[in] rawValueY The raw y-axis input value to simulate.
        virtual void SimulateRawInput2D(float /*rawValueX*/,
                                        float /*rawValueY*/) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Simulate a raw input event. Please use with caution; it's designed primarily for testing
        //! purposes, and could result in strange behaviour if called while the user is interacting
        //! physically with any input device that happens to be updating the same input channel(s),
        //! or if used to simulate input of an input channel whose value is derived from the value
        //! of a different input channel (eg. the InputDeviceGamepad::ThumbStickDirection::* input
        //! channel values are derived from their respective InputDeviceGamepad::ThumbStickAxis2D).
        //!
        //! If used, it is the responsibility of the caller to reset the input channel back to it's
        //! original idle state, otherwise it may be left in a state of being permanently 'active'.
        //!
        //! \param[in] rawValueX The raw x-axis input value to simulate.
        //! \param[in] rawValueY The raw y-axis input value to simulate.
        //! \param[in] rawValueZ The raw z-axis input value to simulate.
        virtual void SimulateRawInput3D(float /*rawValueX*/,
                                        float /*rawValueY*/,
                                        float /*rawValueZ*/) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Simulate a raw input event. Please use with caution; it's designed primarily for testing
        //! purposes, and could result in strange behaviour if called while the user is interacting
        //! physically with any input device that happens to be updating the same input channel(s),
        //! or if used to simulate input of an input channel whose value is derived from the value
        //! of a different input channel (eg. the InputDeviceGamepad::ThumbStickDirection::* input
        //! channel values are derived from their respective InputDeviceGamepad::ThumbStickAxis2D).
        //!
        //! If used, it is the responsibility of the caller to reset the input channel back to it's
        //! original idle state, otherwise it may be left in a state of being permanently 'active'.
        //!
        //! \param[in] rawValue The raw input value to simulate. Analog input channels will use the
        //! value directly, digital input channels treat 0.0f as 'off' and all other values as 'on'.
        //! \param[in] normalizedX The normalized x position of the simulated raw input event.
        //! \param[in] normalizedY The normalized y position of the simulated raw input event.
        virtual void SimulateRawInputWithPosition2D(float /*rawValue*/,
                                                    float /*normalizedX*/,
                                                    float /*normalizedY*/) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputChannelRequests() = default;
    };
    using InputChannelRequestBus = AZ::EBus<InputChannelRequests>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool InputChannelRequests::BusIdType::operator==(const InputChannelRequests::BusIdType& other) const
    {
        return (m_channelId == other.m_channelId) && (m_deviceIndex == other.m_deviceIndex);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool InputChannelRequests::BusIdType::operator!=(const BusIdType& other) const
    {
        return !(*this == other);
    }

} // namespace AzFramework

DECLARE_EBUS_EXTERN(AzFramework::InputChannelRequests);
