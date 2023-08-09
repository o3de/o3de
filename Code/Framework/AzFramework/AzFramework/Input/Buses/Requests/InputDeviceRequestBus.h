/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class InputDevice;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to query input devices for their associated input channels and state
    class InputDeviceRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId so that they are only
        //! handled by one input device that has connected to the bus using that unique id, or they
        //! can be broadcast to all input devices that have connected to the bus, regardless of id.
        //! Connected input devices are ordered by their local player index from lowest to highest.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ByIdAndOrdered;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests should be handled by only one input device connected to each id
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId
        using BusIdType = InputDeviceId;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests are handled by connected devices in the order of local player index
        using BusIdOrderCompare = AZStd::less<BusIdType>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: InputDeviceRequestBus can be accessed from multiple threads, but is safe to use with
        //! LocklessDispatch because connect/disconnect is handled only on engine startup/shutdown (InputSystemComponent).
        using MutexType = AZStd::recursive_mutex;
        static const bool LocklessDispatch = true;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Alias for verbose container class
        using InputDeviceIdSet = AZStd::unordered_set<InputDeviceId>;
        using InputChannelIdSet = AZStd::unordered_set<InputChannelId>;
        using InputDeviceByIdMap = AZStd::unordered_map<InputDeviceId, const InputDevice*>;
        using InputChannelByIdMap = AZStd::unordered_map<InputChannelId, const InputChannel*>;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Finds a specific input device (convenience function)
        //! \param[in] deviceId Id of the input device to find
        //! \return Pointer to the input device if it was found, nullptr if it was not
        static const InputDevice* FindInputDevice(const InputDeviceId& deviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request the ids of all input channels (optionally those associated with an input device)
        //! that return custom data of a specific type (InputChannel::GetCustomData<CustomDataType>).
        //! \param[out] o_channelIds The set of input channel ids to return
        //! \param[in] deviceId (optional) Id of a specific input device to query for input channels
        //! \tparam CustomDataType Only consider input channels that return custom data of this type
        template<class CustomDataType>
        static void GetInputChannelIdsWithCustomDataOfType(InputChannelIdSet& o_channelIds,
                                                           const InputDeviceId* deviceId = nullptr);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Gets the input device that is uniquely identified by the InputDeviceId used to address
        //! the call to this EBus function. Calls to this EBus method should never be broadcast to
        //! all connected input devices, otherwise the device returned will effectively be random.
        //! \return Pointer to the input device if it exists, nullptr otherwise
        virtual const InputDevice* GetInputDevice() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request the ids of all currently enabled input devices. This does not imply they are all
        //! connected, or even available on the current platform, just that they are enabled for the
        //! application (meaning they will generate input when available / connected to the system).
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices will add their id to o_deviceIds)
        //! - EBus<>::Event(id) (the given device will add its id to o_deviceIds - not very useful!)
        //!
        //! \param[out] o_deviceIds The set of input device ids to return
        virtual void GetInputDeviceIds(InputDeviceIdSet& o_deviceIds) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request a map of all currently enabled input devices by id. This does not imply they are
        //! connected, or even available on the current platform, just that they are enabled for the
        //! application (meaning they will generate input when available / connected to the system).
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices will add themselves to o_devicesById)
        //! - EBus<>::Event(id) (the given input device will add itself to o_devicesById)
        //!
        //! \param[out] o_devicesById The map of input devices (keyed by their id) to return
        virtual void GetInputDevicesById(InputDeviceByIdMap& o_devicesById) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request a map of all currently enabled input devices (keyed by their id) that have been
        //! assigned to the specified local user id.
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices assigned to localUserId added to o_devicesById)
        //! - EBus<>::Event(id) (add given input device to o_devicesById if assigned to localUserId)
        //!
        //! \param[out] o_devicesById The map of input devices (keyed by their id) to return
        //! \param[in] localUserId The local user id to check whether input devices are assigned to
        virtual void GetInputDevicesByIdWithAssignedLocalUserId(InputDeviceByIdMap& o_devicesById,
                                                                LocalUserId localUserId) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request the ids of all input channels associated with an input device.
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices will add all their channel ids to o_channelIds)
        //! - EBus<>::Event(id) (the given device will add all of its channel ids to o_channelIds)
        //!
        //! \param[out] o_channelIds The set of input channel ids to return
        virtual void GetInputChannelIds(InputChannelIdSet& o_channelIds) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request all input channels associated with an input device.
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices will add all their channels to o_channelsById)
        //! - EBus<>::Event(id) (the given device will add all of its channels to o_channelsById)
        //!
        //! \param[out] o_channelsById The map of input channels (keyed by their id) to return
        virtual void GetInputChannelsById(InputChannelByIdMap& o_channelsById) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request the text displayed on the physical key / button associated with an input channel.
        //! In the case of keyboard keys, this should take into account the current keyboard layout.
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices will search their channels for inputChannelId)
        //! - EBus<>::Event(id) (the given device will search its channel for inputChannelId)
        //!
        //! \param[in] inputChannelId The input channel id whose key or button text to search for
        //! \param[out] o_keyOrButtonText The text displayed on the physical key or button if found
        virtual void GetPhysicalKeyOrButtonText(const InputChannelId& /*inputChannelId*/,
                                                AZStd::string& /*o_keyOrButtonText*/) const {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Tick/update input devices.
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices are ticked/updated)
        //! - EBus<>::Event(id) (the given device is ticked/updated)
        virtual void TickInputDevice() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputDeviceRequests() = default;
    };
    using InputDeviceRequestBus = AZ::EBus<InputDeviceRequests>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class CustomDataType>
    inline void InputDeviceRequests::GetInputChannelIdsWithCustomDataOfType(
        InputChannelIdSet& o_channelIds,
        const InputDeviceId* deviceId)
    {
        InputChannelByIdMap inputChannelsById;
        if (deviceId)
        {
            InputDeviceRequestBus::Event(*deviceId, &InputDeviceRequests::GetInputChannelsById, inputChannelsById);
        }
        else
        {
            InputDeviceRequestBus::Broadcast(&InputDeviceRequests::GetInputChannelsById, inputChannelsById);
        }

        for (const auto& inputChannelById : inputChannelsById)
        {
            if (inputChannelById.second->GetCustomData<CustomDataType>() != nullptr)
            {
                o_channelIds.insert(inputChannelById.first);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Templated EBus interface used to create a custom implementation for a specific device type
    template<class InputDeviceType>
    class InputDeviceImplementationRequest : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId so that they are only
        //! handled by one input device that has connected to the bus using that unique id, or they
        //! can be broadcast to all input devices that have connected to the bus, regardless of id.
        //! Connected input devices are ordered by their local player index from lowest to highest.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ByIdAndOrdered;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests should be handled by only one input device connected to each id
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId
        using BusIdType = InputDeviceId;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests are handled by connected devices in the order of local player index
        using BusIdOrderCompare = AZStd::less<BusIdType>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for the EBus implementation of this interface
        using Bus = AZ::EBus<InputDeviceImplementationRequest<InputDeviceType>>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set a custom implementation for this input device type, either for a specific instance
        //! by addressing the call to an InputDeviceId, or for all existing instances by broadcast.
        //! Passing InputDeviceType::Implementation::Create as the argument will create the default
        //! device implementation, while passing nullptr will delete any existing implementation.
        //! \param[in] implementationFactory Pointer to the function that creates the implementation.
        virtual void SetCustomImplementation(typename InputDeviceType::ImplementationFactory* implementationFactory) = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Templated EBus handler class that implements the InputDeviceImplementationRequest interface.
    //! To use this helper class your InputDeviceType class must posses all of the following traits,
    //! and they must all be accessible (either by being public or by making this helper a friend):
    //! - A nested InputDeviceType::Implementation class
    //! - A SetImplementation(AZStd::unique_ptr<InputDeviceType::Implementation>) function
    template<class InputDeviceType>
    class InputDeviceImplementationRequestHandler
        : public InputDeviceImplementationRequest<InputDeviceType>::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device that owns this handler
        AZ_INLINE InputDeviceImplementationRequestHandler(InputDeviceType& inputDevice)
            : m_inputDevice(inputDevice)
        {
            InputDeviceImplementationRequest<InputDeviceType>::Bus::Handler::BusConnect(m_inputDevice.GetInputDeviceId());
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputDeviceImplementationRequestHandler);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref InputDeviceImplementationRequest<InputDeviceType>::SetCustomImplementation
        AZ_INLINE void SetCustomImplementation(typename InputDeviceType::ImplementationFactory* implementationFactory) override
        {
            AZStd::unique_ptr<typename InputDeviceType::Implementation> newImplementation;
            newImplementation = (implementationFactory != nullptr) ? implementationFactory->Create(m_inputDevice) : nullptr;
            if (newImplementation)
            {
                m_inputDevice.SetImplementation(AZStd::move(newImplementation));
            }
        }

    private:
        InputDeviceType& m_inputDevice; //!< Reference to the input device that owns this handler
    };
} // namespace AzFramework

DECLARE_EBUS_EXTERN(AzFramework::InputDeviceRequests);
