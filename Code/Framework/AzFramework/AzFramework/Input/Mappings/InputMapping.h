/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/InputDevice.h>

#include <AzCore/std/containers/unordered_set.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class InputContext;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Base class for all input mappings that listen for 'raw' input and output custom input events.
    //! Derived classes should provide additional functions that allow their parent input context to
    //! update the state and value(s) of the input mapping as raw input is received from the system,
    //! and they can (optionally) override the virtual GetCustomData function to return custom data.
    class InputMapping : public InputChannel
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputMapping, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputMapping, "{37CA842F-F6E4-47CD-947A-C9B82A2A4DA2}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input mapping
        //! \param[in] inputContext Input context that owns the input mapping
        InputMapping(const InputChannelId& inputChannelId,
                     const InputContext& inputContext);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputMapping);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default Destructor
        ~InputMapping() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process a potential source input event, that will in turn call OnSourceInputEvent if the
        //! provided input channel is a source input for this input mapping.
        //! \param[in] inputChannel The input channel that is a potential source input
        //! \return True if the input is a source and the event has been consumed, false otherwise
        bool ProcessPotentialSourceInputEvent(const InputChannel& inputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Tick/update the input mapping
        virtual void OnTick();

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to return whether an input channel is a source input for this input mapping.
        //! \param[in] inputChannel The input channel that is a potential source input
        //! \return True if the input channel is a source input, false otherwise
        virtual bool IsSourceInput(const InputChannel& inputChannel) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a source input is active or its state or value is updated.
        //! \param[in] inputChannel The input channel that is active or whose state or value updated
        //! \return True if the input event has been consumed, false otherwise
        virtual bool OnSourceInputEvent(const InputChannel& inputChannel) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Alias for verbose container class
        using SourceInputChannelIds = AZStd::unordered_set<InputChannelId>;
        using ActiveInputChannelIds = AZStd::unordered_map<InputChannelId, InputDeviceId>;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Empty snapshot of an input channel used as the 'default' state for some input mappings.
        struct EmptySnapshot : public Snapshot
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            EmptySnapshot() : Snapshot(InputChannelId(), InputDeviceId(""), State::Idle) {}
        };
    };
} // namespace AzFramework
