/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Requests/InputTextEntryRequestBus.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/InputDevice.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Defines a generic virtual keyboard device. Platform specific implementations are defined as
    //! private implementations so that creating an instance of this generic class will work on any
    //! platform that supports a virtual keyboard.
    class InputDeviceVirtualKeyboard : public InputDevice
                                     , public InputTextEntryRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The id used to identify the primary virtual keyboard input device
        static constexpr inline InputDeviceId Id{"virtual_keyboard"};

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Check whether an input device id identifies a virtual keyboard (regardless of index)
        //! \param[in] inputDeviceId The input device id to check
        //! \return True if the input device id identifies a virtual keyboard, false otherwise
        static bool IsVirtualKeyboardDevice(const InputDeviceId& inputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! All the input channel ids that identify virtual keyboard commands which systems may need
        //! to respond to.
        struct Command
        {
            //!< The clear command used to indicate the user wants to clear the active text field
            static constexpr inline InputChannelId EditClear{"virtual_keyboard_edit_enter"};

            //!< The enter/return/close command used to indicate the user has finished text editing
            static constexpr inline InputChannelId EditEnter{"virtual_keyboard_edit_clear"};

            //!< The back command used to indicate the user wants to navigate 'backwards'.
            //!< This is specific to android devices, and does not have an ios equivalent.
            static constexpr inline InputChannelId NavigationBack{"virtual_keyboard_navigation_back"};

            //!< All virtual keyboard command ids
            static constexpr inline AZStd::array All
            {
                EditClear,
                EditEnter,
                NavigationBack
            };
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceVirtualKeyboard, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputDeviceVirtualKeyboard, "{85BA81F4-EB74-4CFB-8504-DD8C555D8D79}", InputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Foward declare the internal Implementation class so its unique ptr can be referenced from 
        // the ImplementationFactory
        class Implementation;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The factory class to create a custom implementation for this input device
        class ImplementationFactory
        {
        public:
            AZ_TYPE_INFO(ImplementationFactory, "{A6440C08-1367-4F45-87E4-5D85B3DA64E4}");
            virtual ~ImplementationFactory() = default;
            virtual AZStd::unique_ptr<Implementation> Create(InputDeviceVirtualKeyboard& inputDevice) = 0;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDeviceId Optional override of the default input device id
        //! \param[in] implementationFactory Optional override of the default Implementation::Create
        explicit InputDeviceVirtualKeyboard(const InputDeviceId& inputDeviceId = Id,
                                            ImplementationFactory* implementationFactory = AZ::Interface<ImplementationFactory>::Get());

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputDeviceVirtualKeyboard);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceVirtualKeyboard() override;

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
        //! \ref AzFramework::InputTextEntryRequests::HasTextEntryStarted
        bool HasTextEntryStarted() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputTextEntryRequests::TextEntryStart
        void TextEntryStart(const VirtualKeyboardOptions& options) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputTextEntryRequests::TextEntryStop
        void TextEntryStop() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::TickInputDevice
        void TickInputDevice() override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for verbose container class
        using CommandChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannel*>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        InputChannelByIdMap   m_allChannelsById;     //!< All virtual keyboard channels by id
        CommandChannelByIdMap m_commandChannelsById; //!< All virtual command channels by id

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base class for platform specific implementations of virtual keyboard input devices
        class Implementation
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default factory create function
            //! \param[in] inputDevice Reference to the input device being implemented
            static Implementation* Create(InputDeviceVirtualKeyboard& inputDevice);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] inputDevice Reference to the input device being implemented
            Implementation(InputDeviceVirtualKeyboard& inputDevice);

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
            //! Query whether the virtual keyboard is being displayed/text input enabled
            //! \return True if  the virtual keyboard is being displayed, false otherwise
            virtual bool HasTextEntryStarted() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Display the virtual keyboard, enabling text input (pair with StopTextInput)
            //! \param[in] options Used to specify the appearance/behavior of the virtual keyboard
            virtual void TextEntryStart(const VirtualKeyboardOptions& options) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Hide the virtual keyboard, disabling text input (pair with StartTextInput)
            virtual void TextEntryStop() = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Tick/update the input device to broadcast all input events since the last frame
            virtual void TickInputDevice() = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Queue raw command event to be processed in the next call to ProcessRawEventQueues.
            //! This function is not thread safe and so should only be called from the main thread.
            //! \param[in] inputChannelId The input channel id
            void QueueRawCommandEvent(const InputChannelId& inputChannelId);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Queue raw text events to be processed in the next call to ProcessRawEventQueues.
            //! This function is not thread safe and so should only be called from the main thread.
            //! \param[in] textUTF8 The text to queue (encoded using UTF-8)
            void QueueRawTextEvent(const AZStd::string& textUTF8);

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process raw input events that have been queued since the last call to this function.
            //! This function is not thread safe, and so should only be called from the main thread.
            void ProcessRawEventQueues();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Reset the state of all this input device's associated input channels
            void ResetInputChannelStates();

        private:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            InputDeviceVirtualKeyboard&   m_inputDevice;          //!< Reference to the input device
            AZStd::vector<InputChannelId> m_rawCommandEventQueue; //!< The raw command event queue
            AZStd::vector<AZStd::string>  m_rawTextEventQueue;    //!< The raw text event queue
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
        InputDeviceImplementationRequestHandler<InputDeviceVirtualKeyboard> m_implementationRequestHandler;
    };
} // namespace AzFramework
