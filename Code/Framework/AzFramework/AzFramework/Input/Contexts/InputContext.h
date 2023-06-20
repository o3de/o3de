/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Devices/InputDevice.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class InputMapping;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! An InputContext is an InputEventListener that owns a collection of InputMapping objects and
    //! forwards input events to them. By inheriting from InputEventListener the InputContext gains
    //! access to the same priority and 'consumed' systems that all other input event listeners use,
    //! meaning they can be interleaved with any other engine / gameplay system that consumes input.
    //! InputContext also inherits from InputDevice, which while unintuitive is necessary for input
    //! mapping instances that need to be created by passing a reference to the parent input device.
    class InputContext : public InputDevice
                       , public InputChannelEventListener
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Custom data struct used to initialize input contexts
        struct InitData
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            //! The filter used to determine whether an input event should be handled
            AZStd::shared_ptr<InputChannelEventFilter> filter;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The priority used to sort relative to other input event listeners
            AZ::s32 priority = InputChannelEventListener::GetPriorityDefault();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Whether to activate (connect to the input notification bus) on construction
            bool autoActivate = false;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Should the input context consume input that is processed by any of its input mappings?
            bool consumesProcessedInput = false;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputContext, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputContext, "{D17A85B2-405F-40AB-BBA7-F118256D39AB}", InputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] name Unique, will be truncated if exceeds InputDeviceId::MAX_NAME_LENGTH = 64
        explicit InputContext(const char* name);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] name Unique, will be truncated if exceeds InputDeviceId::MAX_NAME_LENGTH = 64
        //! \param[in] initData Custom data struct used to initialize the input context
        InputContext(const char* name, const InitData& initData);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputContext);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputContext() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Activate the input context (this just calls the base InputChannelEventListener::Connect)
        void Activate();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Deactivate the input context (this just calls the base InputChannelEventListener::Disconnect)
        void Deactivate();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input mapping to this input context (the input context will share ownership of it)
        //! \param[in] inputMapping Shared pointer to the input mapping to add to this input context
        //! \return True if the input mapping was added to this input context, false otherwise
        bool AddInputMapping(AZStd::shared_ptr<InputMapping> inputMapping);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Remove an input mapping from this input context (the shared ownership will be released)
        //! \param[in] inputMappingId The id of the input mapping to remove from this input context
        //! \return True if the input mapping was removed from this input context, false otherwise
        bool RemoveInputMapping(const InputChannelId& inputMappingId);

    protected:
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
        //! \ref AzFramework::InputChannelEventListener::OnInputChannelEventFiltered
        bool OnInputChannelEventFiltered(const InputChannel& inputChannel) override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for verbose container class
        using InputMappingsByIdMap = AZStd::unordered_map<InputChannelId, AZStd::shared_ptr<InputMapping>>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! All input channels/mappings owned by this input context. We need to store two separate
        //! containers so that we can return the correct type from GetInputChannelsById while also
        //! maintaining shared ownership of input mappings added via the AddInputMapping function.
        InputChannelByIdMap  m_inputChannelsById;
        InputMappingsByIdMap m_inputMappingsById;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Should this input context consume input that is processed by any of its input mappings?
        bool m_consumesProcessedInput = false;
    };
} // namespace AzFramework
