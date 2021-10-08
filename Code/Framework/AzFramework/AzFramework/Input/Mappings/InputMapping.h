/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/InputDevice.h>

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

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
        //! Convenience class that allows for selection of an input channel name filtered by device.
        struct InputChannelNameFilteredByDeviceType
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(InputChannelNameFilteredByDeviceType, AZ::SystemAllocator, 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Type Info
            AZ_RTTI(InputChannelNameFilteredByDeviceType, "{68CC4865-1C0E-4E2E-BDAE-AF42EA30DBE8}");

            ////////////////////////////////////////////////////////////////////////////////////////
            // Reflection
            static void Reflect(AZ::ReflectContext* context);

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            InputChannelNameFilteredByDeviceType();

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Destructor
            virtual ~InputChannelNameFilteredByDeviceType() = default;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Get the currently selected input device type.
            //! \return Currently selected input device type.
            inline const AZStd::string& GetInputDeviceType() const { return m_inputDeviceType; }

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Get the currently selected input channel name.
            //! \return Currently selected input channel name.
            inline const AZStd::string& GetInputChannelName() const { return m_inputChannelName; }

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Called when an input device type is selected.
            //! \return The AZ::Edit::PropertyRefreshLevels to apply to the property tree view.
            virtual AZ::Crc32 OnInputDeviceTypeSelected();

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Get the name label override to display.
            //! \return Name label override to display.
            virtual AZStd::string GetNameLabelOverride() const;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Get the valid input device types for this input mapping.
            //! \return Valid input device types for this input mapping.
            virtual AZStd::vector<AZStd::string> GetValidInputDeviceTypes() const;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Get the valid input channel names for this input mapping given the selected device type.
            //! \return Valid input channel names for this input mapping given the selected device type.
            virtual AZStd::vector<AZStd::string> GetValidInputChannelNamesBySelectedDevice() const;

        private:
            ////////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            AZStd::string m_inputDeviceType;    //!< The currently selected input device type.
            AZStd::string m_inputChannelName;   //!< The currently selected input channel name.
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base class for input mapping configuration values that are exposed to the editor.
        class ConfigBase
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(ConfigBase, AZ::SystemAllocator, 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Type Info
            AZ_RTTI(ConfigBase, "{72EBBBCC-D57E-4085-AFD9-4910506010B6}");

            ////////////////////////////////////////////////////////////////////////////////////////
            // Reflection
            static void Reflect(AZ::ReflectContext* context);

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Destructor
            virtual ~ConfigBase() = default;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Create an input mapping and add it to the input context.
            //! \param[in] inputContext Input context that the input mapping will be added to.
            AZStd::shared_ptr<InputMapping> CreateInputMappingAndAddToContext(InputContext& inputContext) const;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Override to create the relevant input mapping.
            //! \param[in] inputContext Input context that owns the input mapping.
            virtual AZStd::shared_ptr<InputMapping> CreateInputMapping(const InputContext& inputContext) const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Get the name label override to display.
            //! \return Name label override to display.
            virtual AZStd::string GetNameLabelOverride() const;

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! The unique input channel name (event) output by the input mapping.
            AZStd::string m_outputInputChannelName;
        };

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
