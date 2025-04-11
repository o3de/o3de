/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannelDigital.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    enum class ModifierKeyMask : int
    {
        None     = 0x0000,
        AltL     = 0x0001,
        AltR     = 0x0002,
        CtrlL    = 0x0004,
        CtrlR    = 0x0008,
        ShiftL   = 0x0010,
        ShiftR   = 0x0020,
        SuperL   = 0x0040,
        SuperR   = 0x0080,

        AltAny   = (AltL | AltR),
        CtrlAny  = (CtrlL | CtrlR),
        ShiftAny = (ShiftL | ShiftR),
        SuperAny = (SuperL | SuperR)
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Custom data struct to store the current state of all modifier keys
    struct ModifierKeyStates : public InputChannel::CustomData
    {
        AZ_CLASS_ALLOCATOR(ModifierKeyStates, AZ::SystemAllocator);
        AZ_RTTI(ModifierKeyStates, "{999937EC-6BFD-41F4-A0F2-7990018D3589}", CustomData);
        virtual ~ModifierKeyStates() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the bitmask of active modifier keys
        //! \return The bitmask of active modifier keys
        ModifierKeyMask GetActiveModifierKeys() const { return m_activeModifierKeys; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query whether the specified modifier key is active
        //! \param[in] modifierKey The modifier key to check
        //! \return True if the modifier key is active, false otherwise
        bool IsActive(ModifierKeyMask modifierKey) const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the active state of the specified modifier key
        //! \param[in] modifierKey The modifier key to set
        //! \param[in] active The active state to set
        void SetActive(ModifierKeyMask modifierKey, bool active);

    private:
        ModifierKeyMask m_activeModifierKeys = ModifierKeyMask::None; //!< Active modifier keys
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Alias for verbose shared_ptr class
    using SharedModifierKeyStates = AZStd::shared_ptr<ModifierKeyStates>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class for input channels that emit digital input values and a shared modifier key state.
    //! Examples: keyboard key
    class InputChannelDigitalWithSharedModifierKeyStates : public InputChannelDigital
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelDigitalWithSharedModifierKeyStates, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelDigitalWithSharedModifierKeyStates, "{DAA5C9F4-B833-4F3D-AED5-B8B87BB8FF72}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        //! \param[in] sharedModifierKeyStates The shared modifier key states
        //! \param[in] correspondingModifierKey The corresponding modifier key
        explicit InputChannelDigitalWithSharedModifierKeyStates(
            const InputChannelId& inputChannelId,
            const InputDevice& inputDevice,
            SharedModifierKeyStates& sharedModifierKeyStates,
            ModifierKeyMask correspondingModifierKey = ModifierKeyMask::None);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelDigitalWithSharedModifierKeyStates);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputChannelDigitalWithSharedModifierKeyStates() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the shared modifier key state data associated with the input channel
        //! \return Pointer to the modifier key state data
        const InputChannel::CustomData* GetCustomData() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::ResetState
        void ResetState() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process a raw input event, that will in turn update the channel's state based on whether
        //! it's active/engaged or inactive/idle, broadcasting an input event if the channel is left
        //! in a non-idle state. This function (or InputChannel::UpdateState) should only be called
        //! a max of once per channel per frame from InputDeviceRequests::TickInputDevice to ensure
        //! that input channels broadcast no more than one event each frame (and at the same time).
        //!
        //! Note that this function hides InputChannelDigital::ProcessRawInputEvent which is intended.
        //!
        //! \param[in] rawValues The raw digital value to process
        void ProcessRawInputEvent(bool rawValue);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        SharedModifierKeyStates m_sharedModifierKeyStates;  //!< The shared modifier key states
        ModifierKeyMask         m_correspondingModifierKey; //!< The corresponding modifier key
    };
} // namespace AzFramework
