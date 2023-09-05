/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Utils/ProcessRawInputEventQueues.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboard::IsKeyboardDevice(const InputDeviceId& inputDeviceId)
    {
        return (inputDeviceId.GetNameCrc32() == Id.GetNameCrc32());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ModifierKeyMask GetCorrespondingModifierKeyMask(const InputChannelId& channelId)
    {
        if (channelId == InputDeviceKeyboard::Key::ModifierAltL) { return ModifierKeyMask::AltL; }
        if (channelId == InputDeviceKeyboard::Key::ModifierAltR) { return ModifierKeyMask::AltR; }
        if (channelId == InputDeviceKeyboard::Key::ModifierCtrlL) { return ModifierKeyMask::CtrlL; }
        if (channelId == InputDeviceKeyboard::Key::ModifierCtrlR) { return ModifierKeyMask::CtrlR; }
        if (channelId == InputDeviceKeyboard::Key::ModifierShiftL) { return ModifierKeyMask::ShiftL; }
        if (channelId == InputDeviceKeyboard::Key::ModifierShiftR) { return ModifierKeyMask::ShiftR; }
        if (channelId == InputDeviceKeyboard::Key::ModifierSuperL) { return ModifierKeyMask::SuperL; }
        if (channelId == InputDeviceKeyboard::Key::ModifierSuperR) { return ModifierKeyMask::SuperR; }
        return ModifierKeyMask::None;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // Unfortunately it doesn't seem possible to reflect anything through BehaviorContext
            // using lambdas which capture variables from the enclosing scope. So we are manually
            // reflecting all input channel names, instead of just iterating over them like this:
            //
            //  auto classBuilder = behaviorContext->Class<InputDeviceKeyboard>();
            //  for (const InputChannelId& channelId : Key::All)
            //  {
            //      const char* channelName = channelId.GetName();
            //      classBuilder->Constant(channelName, [channelName]() { return channelName; });
            //  }

            behaviorContext->Class<InputDeviceKeyboard>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Constant("name", BehaviorConstant(Id.GetName()))

                ->Constant(Key::Alphanumeric0.GetName(), BehaviorConstant(Key::Alphanumeric0.GetName()))
                ->Constant(Key::Alphanumeric1.GetName(), BehaviorConstant(Key::Alphanumeric1.GetName()))
                ->Constant(Key::Alphanumeric2.GetName(), BehaviorConstant(Key::Alphanumeric2.GetName()))
                ->Constant(Key::Alphanumeric3.GetName(), BehaviorConstant(Key::Alphanumeric3.GetName()))
                ->Constant(Key::Alphanumeric4.GetName(), BehaviorConstant(Key::Alphanumeric4.GetName()))
                ->Constant(Key::Alphanumeric5.GetName(), BehaviorConstant(Key::Alphanumeric5.GetName()))
                ->Constant(Key::Alphanumeric6.GetName(), BehaviorConstant(Key::Alphanumeric6.GetName()))
                ->Constant(Key::Alphanumeric7.GetName(), BehaviorConstant(Key::Alphanumeric7.GetName()))
                ->Constant(Key::Alphanumeric8.GetName(), BehaviorConstant(Key::Alphanumeric8.GetName()))
                ->Constant(Key::Alphanumeric9.GetName(), BehaviorConstant(Key::Alphanumeric9.GetName()))
                ->Constant(Key::AlphanumericA.GetName(), BehaviorConstant(Key::AlphanumericA.GetName()))
                ->Constant(Key::AlphanumericB.GetName(), BehaviorConstant(Key::AlphanumericB.GetName()))
                ->Constant(Key::AlphanumericC.GetName(), BehaviorConstant(Key::AlphanumericC.GetName()))
                ->Constant(Key::AlphanumericD.GetName(), BehaviorConstant(Key::AlphanumericD.GetName()))
                ->Constant(Key::AlphanumericE.GetName(), BehaviorConstant(Key::AlphanumericE.GetName()))
                ->Constant(Key::AlphanumericF.GetName(), BehaviorConstant(Key::AlphanumericF.GetName()))
                ->Constant(Key::AlphanumericG.GetName(), BehaviorConstant(Key::AlphanumericG.GetName()))
                ->Constant(Key::AlphanumericH.GetName(), BehaviorConstant(Key::AlphanumericH.GetName()))
                ->Constant(Key::AlphanumericI.GetName(), BehaviorConstant(Key::AlphanumericI.GetName()))
                ->Constant(Key::AlphanumericJ.GetName(), BehaviorConstant(Key::AlphanumericJ.GetName()))
                ->Constant(Key::AlphanumericK.GetName(), BehaviorConstant(Key::AlphanumericK.GetName()))
                ->Constant(Key::AlphanumericL.GetName(), BehaviorConstant(Key::AlphanumericL.GetName()))
                ->Constant(Key::AlphanumericM.GetName(), BehaviorConstant(Key::AlphanumericM.GetName()))
                ->Constant(Key::AlphanumericN.GetName(), BehaviorConstant(Key::AlphanumericN.GetName()))
                ->Constant(Key::AlphanumericO.GetName(), BehaviorConstant(Key::AlphanumericO.GetName()))
                ->Constant(Key::AlphanumericP.GetName(), BehaviorConstant(Key::AlphanumericP.GetName()))
                ->Constant(Key::AlphanumericQ.GetName(), BehaviorConstant(Key::AlphanumericQ.GetName()))
                ->Constant(Key::AlphanumericR.GetName(), BehaviorConstant(Key::AlphanumericR.GetName()))
                ->Constant(Key::AlphanumericS.GetName(), BehaviorConstant(Key::AlphanumericS.GetName()))
                ->Constant(Key::AlphanumericT.GetName(), BehaviorConstant(Key::AlphanumericT.GetName()))
                ->Constant(Key::AlphanumericU.GetName(), BehaviorConstant(Key::AlphanumericU.GetName()))
                ->Constant(Key::AlphanumericV.GetName(), BehaviorConstant(Key::AlphanumericV.GetName()))
                ->Constant(Key::AlphanumericW.GetName(), BehaviorConstant(Key::AlphanumericW.GetName()))
                ->Constant(Key::AlphanumericX.GetName(), BehaviorConstant(Key::AlphanumericX.GetName()))
                ->Constant(Key::AlphanumericY.GetName(), BehaviorConstant(Key::AlphanumericY.GetName()))
                ->Constant(Key::AlphanumericZ.GetName(), BehaviorConstant(Key::AlphanumericZ.GetName()))

                ->Constant(Key::EditBackspace.GetName(), BehaviorConstant(Key::EditBackspace.GetName()))
                ->Constant(Key::EditCapsLock.GetName(), BehaviorConstant(Key::EditCapsLock.GetName()))
                ->Constant(Key::EditEnter.GetName(), BehaviorConstant(Key::EditEnter.GetName()))
                ->Constant(Key::EditSpace.GetName(), BehaviorConstant(Key::EditSpace.GetName()))
                ->Constant(Key::EditTab.GetName(), BehaviorConstant(Key::EditTab.GetName()))
                ->Constant(Key::Escape.GetName(), BehaviorConstant(Key::Escape.GetName()))

                ->Constant(Key::Function01.GetName(), BehaviorConstant(Key::Function01.GetName()))
                ->Constant(Key::Function02.GetName(), BehaviorConstant(Key::Function02.GetName()))
                ->Constant(Key::Function03.GetName(), BehaviorConstant(Key::Function03.GetName()))
                ->Constant(Key::Function04.GetName(), BehaviorConstant(Key::Function04.GetName()))
                ->Constant(Key::Function05.GetName(), BehaviorConstant(Key::Function05.GetName()))
                ->Constant(Key::Function06.GetName(), BehaviorConstant(Key::Function06.GetName()))
                ->Constant(Key::Function07.GetName(), BehaviorConstant(Key::Function07.GetName()))
                ->Constant(Key::Function08.GetName(), BehaviorConstant(Key::Function08.GetName()))
                ->Constant(Key::Function09.GetName(), BehaviorConstant(Key::Function09.GetName()))
                ->Constant(Key::Function10.GetName(), BehaviorConstant(Key::Function10.GetName()))
                ->Constant(Key::Function11.GetName(), BehaviorConstant(Key::Function11.GetName()))
                ->Constant(Key::Function12.GetName(), BehaviorConstant(Key::Function12.GetName()))
                ->Constant(Key::Function13.GetName(), BehaviorConstant(Key::Function13.GetName()))
                ->Constant(Key::Function14.GetName(), BehaviorConstant(Key::Function14.GetName()))
                ->Constant(Key::Function15.GetName(), BehaviorConstant(Key::Function15.GetName()))
                ->Constant(Key::Function16.GetName(), BehaviorConstant(Key::Function16.GetName()))
                ->Constant(Key::Function17.GetName(), BehaviorConstant(Key::Function17.GetName()))
                ->Constant(Key::Function18.GetName(), BehaviorConstant(Key::Function18.GetName()))
                ->Constant(Key::Function19.GetName(), BehaviorConstant(Key::Function19.GetName()))
                ->Constant(Key::Function20.GetName(), BehaviorConstant(Key::Function20.GetName()))

                ->Constant(Key::ModifierAltL.GetName(), BehaviorConstant(Key::ModifierAltL.GetName()))
                ->Constant(Key::ModifierAltR.GetName(), BehaviorConstant(Key::ModifierAltR.GetName()))
                ->Constant(Key::ModifierCtrlL.GetName(), BehaviorConstant(Key::ModifierCtrlL.GetName()))
                ->Constant(Key::ModifierCtrlR.GetName(), BehaviorConstant(Key::ModifierCtrlR.GetName()))
                ->Constant(Key::ModifierShiftL.GetName(), BehaviorConstant(Key::ModifierShiftL.GetName()))
                ->Constant(Key::ModifierShiftR.GetName(), BehaviorConstant(Key::ModifierShiftR.GetName()))
                ->Constant(Key::ModifierSuperL.GetName(), BehaviorConstant(Key::ModifierSuperL.GetName()))
                ->Constant(Key::ModifierSuperR.GetName(), BehaviorConstant(Key::ModifierSuperR.GetName()))

                ->Constant(Key::NavigationArrowDown.GetName(), BehaviorConstant(Key::NavigationArrowDown.GetName()))
                ->Constant(Key::NavigationArrowLeft.GetName(), BehaviorConstant(Key::NavigationArrowLeft.GetName()))
                ->Constant(Key::NavigationArrowRight.GetName(), BehaviorConstant(Key::NavigationArrowRight.GetName()))
                ->Constant(Key::NavigationArrowUp.GetName(), BehaviorConstant(Key::NavigationArrowUp.GetName()))
                ->Constant(Key::NavigationDelete.GetName(), BehaviorConstant(Key::NavigationDelete.GetName()))
                ->Constant(Key::NavigationEnd.GetName(), BehaviorConstant(Key::NavigationEnd.GetName()))
                ->Constant(Key::NavigationHome.GetName(), BehaviorConstant(Key::NavigationHome.GetName()))
                ->Constant(Key::NavigationInsert.GetName(), BehaviorConstant(Key::NavigationInsert.GetName()))
                ->Constant(Key::NavigationPageDown.GetName(), BehaviorConstant(Key::NavigationPageDown.GetName()))
                ->Constant(Key::NavigationPageUp.GetName(), BehaviorConstant(Key::NavigationPageUp.GetName()))

                ->Constant(Key::NumLock.GetName(), BehaviorConstant(Key::NumLock.GetName()))
                ->Constant(Key::NumPad0.GetName(), BehaviorConstant(Key::NumPad0.GetName()))
                ->Constant(Key::NumPad1.GetName(), BehaviorConstant(Key::NumPad1.GetName()))
                ->Constant(Key::NumPad2.GetName(), BehaviorConstant(Key::NumPad2.GetName()))
                ->Constant(Key::NumPad3.GetName(), BehaviorConstant(Key::NumPad3.GetName()))
                ->Constant(Key::NumPad4.GetName(), BehaviorConstant(Key::NumPad4.GetName()))
                ->Constant(Key::NumPad5.GetName(), BehaviorConstant(Key::NumPad5.GetName()))
                ->Constant(Key::NumPad6.GetName(), BehaviorConstant(Key::NumPad6.GetName()))
                ->Constant(Key::NumPad7.GetName(), BehaviorConstant(Key::NumPad7.GetName()))
                ->Constant(Key::NumPad8.GetName(), BehaviorConstant(Key::NumPad8.GetName()))
                ->Constant(Key::NumPad9.GetName(), BehaviorConstant(Key::NumPad9.GetName()))
                ->Constant(Key::NumPadAdd.GetName(), BehaviorConstant(Key::NumPadAdd.GetName()))
                ->Constant(Key::NumPadDecimal.GetName(), BehaviorConstant(Key::NumPadDecimal.GetName()))
                ->Constant(Key::NumPadDivide.GetName(), BehaviorConstant(Key::NumPadDivide.GetName()))
                ->Constant(Key::NumPadEnter.GetName(), BehaviorConstant(Key::NumPadEnter.GetName()))
                ->Constant(Key::NumPadMultiply.GetName(), BehaviorConstant(Key::NumPadMultiply.GetName()))
                ->Constant(Key::NumPadSubtract.GetName(), BehaviorConstant(Key::NumPadSubtract.GetName()))

                ->Constant(Key::PunctuationApostrophe.GetName(), BehaviorConstant(Key::PunctuationApostrophe.GetName()))
                ->Constant(Key::PunctuationBackslash.GetName(), BehaviorConstant(Key::PunctuationBackslash.GetName()))
                ->Constant(Key::PunctuationBracketL.GetName(), BehaviorConstant(Key::PunctuationBracketL.GetName()))
                ->Constant(Key::PunctuationBracketR.GetName(), BehaviorConstant(Key::PunctuationBracketR.GetName()))
                ->Constant(Key::PunctuationComma.GetName(), BehaviorConstant(Key::PunctuationComma.GetName()))
                ->Constant(Key::PunctuationEquals.GetName(), BehaviorConstant(Key::PunctuationEquals.GetName()))
                ->Constant(Key::PunctuationHyphen.GetName(), BehaviorConstant(Key::PunctuationHyphen.GetName()))
                ->Constant(Key::PunctuationPeriod.GetName(), BehaviorConstant(Key::PunctuationPeriod.GetName()))
                ->Constant(Key::PunctuationSemicolon.GetName(), BehaviorConstant(Key::PunctuationSemicolon.GetName()))
                ->Constant(Key::PunctuationSlash.GetName(), BehaviorConstant(Key::PunctuationSlash.GetName()))
                ->Constant(Key::PunctuationTilde.GetName(), BehaviorConstant(Key::PunctuationTilde.GetName()))

                ->Constant(Key::SupplementaryISO.GetName(), BehaviorConstant(Key::SupplementaryISO.GetName()))
                ->Constant(Key::WindowsSystemPause.GetName(), BehaviorConstant(Key::WindowsSystemPause.GetName()))
                ->Constant(Key::WindowsSystemPrint.GetName(), BehaviorConstant(Key::WindowsSystemPrint.GetName()))
                ->Constant(Key::WindowsSystemScrollLock.GetName(), BehaviorConstant(Key::WindowsSystemScrollLock.GetName()))
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboard::InputDeviceKeyboard(const InputDeviceId& inputDeviceId,
                                             ImplementationFactory* implementationFactory)
        : InputDevice(inputDeviceId)
        , m_modifierKeyStates(AZStd::make_shared<ModifierKeyStates>())
        , m_allChannelsById()
        , m_keyChannelsById()
        , m_pimpl()
        , m_implementationRequestHandler(*this)
    {
        // Create all key input channels
        for (const InputChannelId& channelId : Key::All)
        {
            const ModifierKeyMask modifierKeyMask = GetCorrespondingModifierKeyMask(channelId);
            InputChannelDigitalWithSharedModifierKeyStates* channel =
                aznew InputChannelDigitalWithSharedModifierKeyStates(channelId,
                                                                     *this,
                                                                     m_modifierKeyStates,
                                                                     modifierKeyMask);
            m_allChannelsById[channelId] = channel;
            m_keyChannelsById[channelId] = channel;
        }

        // Create the platform specific or custom implementation
        m_pimpl = (implementationFactory != nullptr) ? implementationFactory->Create(*this) : nullptr;

        // Connect to the text entry request bus
        InputTextEntryRequestBus::Handler::BusConnect(GetInputDeviceId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboard::~InputDeviceKeyboard()
    {
        // Disconnect from the text entry request bus
        InputTextEntryRequestBus::Handler::BusDisconnect(GetInputDeviceId());

        // Destroy the platform specific implementation
        m_pimpl.reset();

        // Destroy all key input channels
        for (const auto& channelById : m_keyChannelsById)
        {
            delete channelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    LocalUserId InputDeviceKeyboard::GetAssignedLocalUserId() const
    {
        return m_pimpl ? m_pimpl->GetAssignedLocalUserId() : InputDevice::GetAssignedLocalUserId();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceKeyboard::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboard::IsSupported() const
    {
        return m_pimpl != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboard::IsConnected() const
    {
        return m_pimpl ? m_pimpl->IsConnected() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboard::HasTextEntryStarted() const
    {
        return m_pimpl ? m_pimpl->HasTextEntryStarted() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::TextEntryStart(const VirtualKeyboardOptions& options)
    {
        if (m_pimpl)
        {
            m_pimpl->TextEntryStart(options);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::TextEntryStop()
    {
        if (m_pimpl)
        {
            m_pimpl->TextEntryStop();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::TickInputDevice()
    {
        if (m_pimpl)
        {
            m_pimpl->TickInputDevice();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                                         AZStd::string& o_keyOrButtonText) const
    {
        if (m_pimpl)
        {
            m_pimpl->GetPhysicalKeyOrButtonText(inputChannelId, o_keyOrButtonText);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboard::Implementation::Implementation(InputDeviceKeyboard& inputDevice)
        : m_inputDevice(inputDevice)
        , m_rawKeyEventQueuesById()
        , m_rawTextEventQueue()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboard::Implementation::~Implementation()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    LocalUserId InputDeviceKeyboard::Implementation::GetAssignedLocalUserId() const
    {
        return m_inputDevice.GetInputDeviceId().GetIndex();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::Implementation::QueueRawKeyEvent(const InputChannelId& inputChannelId,
                                                               bool rawKeyState)
    {
        // It should not (in theory) be possible to receive multiple raw key events with the same id
        // and state in succession; if it happens in practice for whatever reason this is still safe.
        m_rawKeyEventQueuesById[inputChannelId].push_back(rawKeyState);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::Implementation::QueueRawTextEvent(const AZStd::string& textUTF8)
    {
        m_rawTextEventQueue.push_back(textUTF8);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::Implementation::ProcessRawEventQueues()
    {
        // Process all raw input events that were queued since the last call to this function.
        // Text events should be processed first in case text input is disabled by a key event.
        ProcessRawInputTextEventQueue(m_rawTextEventQueue);
        ProcessRawInputEventQueues(m_rawKeyEventQueuesById, m_inputDevice.m_keyChannelsById);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::Implementation::ResetInputChannelStates()
    {
        m_inputDevice.ResetInputChannelStates();
    }
} // namespace AzFramework
