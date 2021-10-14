/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gmock/gmock-actions.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <xcb/xcb.h>

#include <AzFramework/XcbApplication.h>
#include <AzFramework/XcbInputDeviceKeyboard.h>
#include <AzFramework/Input/Buses/Notifications/InputTextNotificationBus.h>
#include "MockXcbInterface.h"
#include "Matchers.h"
#include "Actions.h"
#include "XcbBaseTestFixture.h"
#include "XcbTestApplication.h"

template<typename T>
xcb_generic_event_t MakeEvent(T event)
{
    return *reinterpret_cast<xcb_generic_event_t*>(&event);
}

namespace AzFramework
{
    // Sets up default behavior for mock keyboard responses to xcb methods
    class XcbInputDeviceKeyboardTests
        : public XcbBaseTestFixture
    {
    public:
        void SetUp() override
        {
            using testing::Return;
            using testing::SetArgPointee;
            using testing::_;

            XcbBaseTestFixture::SetUp();
            EXPECT_CALL(m_interface, xkb_context_new(XKB_CONTEXT_NO_FLAGS))
                .WillOnce(Return(&m_xkbContext));
            EXPECT_CALL(m_interface, xkb_context_unref(&m_xkbContext))
                .Times(1);

            EXPECT_CALL(m_interface, xkb_x11_keymap_new_from_device(&m_xkbContext, &m_connection, s_coreDeviceId, XKB_KEYMAP_COMPILE_NO_FLAGS))
                .WillOnce(Return(&m_xkbKeymap));
            EXPECT_CALL(m_interface, xkb_keymap_unref(&m_xkbKeymap))
                .Times(1);

            EXPECT_CALL(m_interface, xkb_x11_state_new_from_device(&m_xkbKeymap, &m_connection, s_coreDeviceId))
                .WillOnce(Return(&m_xkbState));
            EXPECT_CALL(m_interface, xkb_state_unref(&m_xkbState))
                .Times(1);

            ON_CALL(m_interface, xkb_x11_setup_xkb_extension(&m_connection, 1, 0, XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, _, _, _, _))
                .WillByDefault(DoAll(
                    SetArgPointee<6>(s_xkbEventCode), // Set the "base_event_out" argument to the xkbEventCode, the value to identify XKB events
                    Return(1)
                ));

            constexpr unsigned int xcbXkbSelectEventsSequence = 342;
            ON_CALL(m_interface, xcb_xkb_select_events(&m_connection, _, _, _, _, _, _, _))
                .WillByDefault(Return(xcb_void_cookie_t{/*.sequence = */ xcbXkbSelectEventsSequence}));
            ON_CALL(m_interface, xcb_request_check(&m_connection, testing::Field(&xcb_void_cookie_t::sequence, testing::Eq(xcbXkbSelectEventsSequence))))
                .WillByDefault(Return(nullptr)); // indicates success

            ON_CALL(m_interface, xkb_x11_get_core_keyboard_device_id(&m_connection))
                .WillByDefault(Return(s_coreDeviceId));

            ON_CALL(m_interface, xkb_state_key_get_one_sym(&m_xkbState, s_keycodeForAKey))
                .WillByDefault(Return(XKB_KEY_a))
                ;
            ON_CALL(m_interface, xkb_state_key_get_one_sym(&m_xkbState, s_keycodeForShiftLKey))
                .WillByDefault(Return(XKB_KEY_Shift_L))
                ;

            ON_CALL(m_interface, xkb_state_update_mask(&m_xkbState, _, _, _, _, _, _))
                .WillByDefault(testing::Invoke(this, &XcbInputDeviceKeyboardTests::UpdateStateMask));

            ON_CALL(m_interface, xkb_state_key_get_utf8(&m_xkbState, s_keycodeForAKey, nullptr, 0))
                .WillByDefault(Return(1));
            ON_CALL(m_interface, xkb_state_key_get_utf8(m_matchesStateWithoutShift, s_keycodeForAKey, _, 2))
                .WillByDefault(DoAll(
                    SetArgPointee<2>('a'),
                    Return(1)
                ));
            ON_CALL(m_interface, xkb_state_key_get_utf8(m_matchesStateWithShift, s_keycodeForAKey, _, 2))
                .WillByDefault(DoAll(
                    SetArgPointee<2>('A'),
                    Return(1)
                ));

            ON_CALL(m_interface, xkb_state_key_get_utf8(&m_xkbState, s_keycodeForShiftLKey, nullptr, 0))
                .WillByDefault(Return(0));
        }

    private:
        xkb_state_component UpdateStateMask(
            xkb_state* state,
            xkb_mod_mask_t depressed_mods,
            xkb_mod_mask_t latched_mods,
            xkb_mod_mask_t locked_mods,
            xkb_layout_index_t depressed_layout,
            xkb_layout_index_t latched_layout,
            xkb_layout_index_t locked_layout)
        {
            state->m_modifiers = depressed_mods | locked_mods;
            return {};
        }

    protected:
        xkb_context m_xkbContext{};
        xkb_keymap m_xkbKeymap{};
        xkb_state m_xkbState{};
        const testing::Matcher<xkb_state*> m_matchesStateWithoutShift = testing::AllOf(&m_xkbState, testing::Field(&xkb_state::m_modifiers, 0));
        const testing::Matcher<xkb_state*> m_matchesStateWithShift = testing::AllOf(&m_xkbState, testing::Field(&xkb_state::m_modifiers, XCB_MOD_MASK_SHIFT));

        static constexpr int32_t s_coreDeviceId{1};
        static constexpr uint8_t s_xkbEventCode{85};

        static constexpr xcb_keycode_t s_keycodeForAKey{38};
        static constexpr xcb_keycode_t s_keycodeForShiftLKey{50};

        XcbTestApplication m_application{
            /*enabledGamepadsCount=*/0,
            /*keyboardEnabled=*/true,
            /*motionEnabled=*/false,
            /*mouseEnabled=*/false,
            /*touchEnabled=*/false,
            /*virtualKeyboardEnabled=*/false
        };
    };

    class InputTextNotificationListener
        : public InputTextNotificationBus::Handler
    {
    public:
        InputTextNotificationListener()
        {
            BusConnect();
        }
        MOCK_METHOD2(OnInputTextEvent, void(const AZStd::string& /*textUTF8*/, bool& /*o_hasBeenConsumed*/));
    };

    TEST_F(XcbInputDeviceKeyboardTests, InputChannelsUpdateStateFromXcbEvents)
    {
        using testing::DoAll;
        using testing::Eq;
        using testing::Return;
        using testing::SetArgPointee;
        using testing::_;

        const AZStd::array events
        {
            MakeEvent(xcb_key_press_event_t{
                /*.response_type = */ XCB_KEY_PRESS,
                /*.detail        = */ s_keycodeForAKey,
                /*.sequence      = */ 0,
                /*.time          = */ 0,
                /*.root          = */ 0,
                /*.event         = */ 0,
                /*.child         = */ 0,
                /*.root_x        = */ 0,
                /*.root_y        = */ 0,
                /*.event_x       = */ 0,
                /*.event_y       = */ 0,
                /*.state         = */ 0,
                /*.same_screen   = */ 0,
                /*.pad0          = */ 0
            }),
            MakeEvent(xcb_key_release_event_t{
                /*.response_type = */ XCB_KEY_RELEASE,
                /*.detail        = */ s_keycodeForAKey,
                /*.sequence      = */ 0,
                /*.time          = */ 0,
                /*.root          = */ 0,
                /*.event         = */ 0,
                /*.child         = */ 0,
                /*.root_x        = */ 0,
                /*.root_y        = */ 0,
                /*.event_x       = */ 0,
                /*.event_y       = */ 0,
                /*.state         = */ 0,
                /*.same_screen   = */ 0,
                /*.pad0          = */ 0
            }),
        };

        // Set the expectations for the events that will be generated
        // nullptr entries represent when the event queue is empty, and will cause
        // PumpSystemEventLoopUntilEmpty to return
        // event pointers are freed by the calling code, so we malloc new copies
        // here
        EXPECT_CALL(m_interface, xcb_poll_for_event(&m_connection))
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[0]))
            .WillOnce(Return(nullptr))
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[1]))
            .WillOnce(Return(nullptr))
            ;

        EXPECT_CALL(m_interface, xkb_state_key_get_one_sym(&m_xkbState, s_keycodeForAKey))
            .Times(2);

        m_application.Start();

        const InputChannel* inputChannel = InputChannelRequests::FindInputChannel(InputDeviceKeyboard::Key::AlphanumericA);
        ASSERT_TRUE(inputChannel);
        EXPECT_THAT(inputChannel->GetState(), Eq(InputChannel::State::Idle));

        m_application.PumpSystemEventLoopUntilEmpty();
        m_application.TickSystem();
        m_application.Tick();

        EXPECT_THAT(inputChannel->GetState(), Eq(InputChannel::State::Began));

        m_application.PumpSystemEventLoopUntilEmpty();
        m_application.TickSystem();
        m_application.Tick();

        EXPECT_THAT(inputChannel->GetState(), Eq(InputChannel::State::Ended));
    }

    TEST_F(XcbInputDeviceKeyboardTests, TextEnteredFromXcbKeyPressEvents)
    {
        using testing::DoAll;
        using testing::Eq;
        using testing::Return;
        using testing::SetArgPointee;
        using testing::_;

        // press a
        // release a
        // press shift
        // press a
        // release a
        // release shift
        const AZStd::array events
        {
            MakeEvent(xcb_key_press_event_t{
                /*.response_type = */ XCB_KEY_PRESS,
                /*.detail        = */ s_keycodeForAKey,
                /*.sequence      = */ 0,
                /*.time          = */ 0,
                /*.root          = */ 0,
                /*.event         = */ 0,
                /*.child         = */ 0,
                /*.root_x        = */ 0,
                /*.root_y        = */ 0,
                /*.event_x       = */ 0,
                /*.event_y       = */ 0,
                /*.state         = */ 0,
                /*.same_screen   = */ 0,
                /*.pad0          = */ 0
            }),
            MakeEvent(xcb_key_release_event_t{
                /*.response_type = */ XCB_KEY_RELEASE,
                /*.detail        = */ s_keycodeForAKey,
                /*.sequence      = */ 0,
                /*.time          = */ 0,
                /*.root          = */ 0,
                /*.event         = */ 0,
                /*.child         = */ 0,
                /*.root_x        = */ 0,
                /*.root_y        = */ 0,
                /*.event_x       = */ 0,
                /*.event_y       = */ 0,
                /*.state         = */ 0,
                /*.same_screen   = */ 0,
                /*.pad0          = */ 0
            }),
            // Pressing a modifier key will generate a key press event followed
            // by a state notify event
            MakeEvent(xcb_key_press_event_t{
                /*.response_type = */ XCB_KEY_PRESS,
                /*.detail        = */ s_keycodeForShiftLKey,
                /*.sequence      = */ 0,
                /*.time          = */ 0,
                /*.root          = */ 0,
                /*.event         = */ 0,
                /*.child         = */ 0,
                /*.root_x        = */ 0,
                /*.root_y        = */ 0,
                /*.event_x       = */ 0,
                /*.event_y       = */ 0,
                /*.state         = */ 0,
                /*.same_screen   = */ 0,
                /*.pad0          = */ 0
            }),
            MakeEvent(xcb_xkb_state_notify_event_t{
                /*.response_type     = */ s_xkbEventCode,
                /*.xkbType           = */ XCB_XKB_STATE_NOTIFY,
                /*.sequence          = */ 0,
                /*.time              = */ 0,
                /*.deviceID          = */ s_coreDeviceId,
                /*.mods              = */ XCB_MOD_MASK_SHIFT,
                /*.baseMods          = */ XCB_MOD_MASK_SHIFT,
                /*.latchedMods       = */ 0,
                /*.lockedMods        = */ 0,
                /*.group             = */ 0,
                /*.baseGroup         = */ 0,
                /*.latchedGroup      = */ 0,
                /*.lockedGroup       = */ 0,
                /*.compatState       = */ XCB_MOD_MASK_SHIFT,
                /*.grabMods          = */ XCB_MOD_MASK_SHIFT,
                /*.compatGrabMods    = */ XCB_MOD_MASK_SHIFT,
                /*.lookupMods        = */ XCB_MOD_MASK_SHIFT,
                /*.compatLoockupMods = */ XCB_MOD_MASK_SHIFT,
                /*.ptrBtnState       = */ 0,
                /*.changed           = */ 0,
                /*.keycode           = */ s_keycodeForShiftLKey,
                /*.eventType         = */ XCB_KEY_PRESS,
                /*.requestMajor      = */ 0,
                /*.requestMinor      = */ 0,
            }),
            MakeEvent(xcb_key_press_event_t{
                /*.response_type = */ XCB_KEY_PRESS,
                /*.detail        = */ s_keycodeForAKey,
                /*.sequence      = */ 0,
                /*.time          = */ 0,
                /*.root          = */ 0,
                /*.event         = */ 0,
                /*.child         = */ 0,
                /*.root_x        = */ 0,
                /*.root_y        = */ 0,
                /*.event_x       = */ 0,
                /*.event_y       = */ 0,
                /*.state         = */ 0,
                /*.same_screen   = */ 0,
                /*.pad0          = */ 0
            }),
            MakeEvent(xcb_key_release_event_t{
                /*.response_type = */ XCB_KEY_RELEASE,
                /*.detail        = */ s_keycodeForAKey,
                /*.sequence      = */ 0,
                /*.time          = */ 0,
                /*.root          = */ 0,
                /*.event         = */ 0,
                /*.child         = */ 0,
                /*.root_x        = */ 0,
                /*.root_y        = */ 0,
                /*.event_x       = */ 0,
                /*.event_y       = */ 0,
                /*.state         = */ 0,
                /*.same_screen   = */ 0,
                /*.pad0          = */ 0
            }),
            MakeEvent(xcb_key_release_event_t{
                /*.response_type = */ XCB_KEY_RELEASE,
                /*.detail        = */ s_keycodeForShiftLKey,
                /*.sequence      = */ 0,
                /*.time          = */ 0,
                /*.root          = */ 0,
                /*.event         = */ 0,
                /*.child         = */ 0,
                /*.root_x        = */ 0,
                /*.root_y        = */ 0,
                /*.event_x       = */ 0,
                /*.event_y       = */ 0,
                /*.state         = */ 0,
                /*.same_screen   = */ 0,
                /*.pad0          = */ 0
            }),
            MakeEvent(xcb_xkb_state_notify_event_t{
                /*.response_type     = */ s_xkbEventCode,
                /*.xkbType           = */ XCB_XKB_STATE_NOTIFY,
                /*.sequence          = */ 0,
                /*.time              = */ 0,
                /*.deviceID          = */ s_coreDeviceId,
                /*.mods              = */ 0,
                /*.baseMods          = */ 0,
                /*.latchedMods       = */ 0,
                /*.lockedMods        = */ 0,
                /*.group             = */ 0,
                /*.baseGroup         = */ 0,
                /*.latchedGroup      = */ 0,
                /*.lockedGroup       = */ 0,
                /*.compatState       = */ 0,
                /*.grabMods          = */ 0,
                /*.compatGrabMods    = */ 0,
                /*.lookupMods        = */ 0,
                /*.compatLoockupMods = */ 0,
                /*.ptrBtnState       = */ 0,
                /*.changed           = */ 0,
                /*.keycode           = */ s_keycodeForShiftLKey,
                /*.eventType         = */ XCB_KEY_RELEASE,
                /*.requestMajor      = */ 0,
                /*.requestMinor      = */ 0,
            }),
        };

        // Set the expectations for the events that will be generated
        // nullptr entries represent when the event queue is empty, and will cause
        // PumpSystemEventLoopUntilEmpty to return
        // event pointers are freed by the calling code, so we malloc new copies
        // here
        EXPECT_CALL(m_interface, xcb_poll_for_event(&m_connection))
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[0])) // press a
            .WillOnce(Return(nullptr))
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[1])) // release a
            .WillOnce(Return(nullptr))
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[2])) // press shift
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[3])) // state notify shift is down
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[4])) // press a
            .WillOnce(Return(nullptr))
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[5])) // release a
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[6])) // release shift
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[7])) // state notify shift is up
            .WillRepeatedly(Return(nullptr))
            ;

        EXPECT_CALL(m_interface, xkb_state_key_get_utf8(&m_xkbState, s_keycodeForAKey, nullptr, 0))
            .Times(2);
        EXPECT_CALL(m_interface, xkb_state_key_get_utf8(m_matchesStateWithoutShift, s_keycodeForAKey, _, 2))
            .Times(1);
        EXPECT_CALL(m_interface, xkb_state_key_get_utf8(m_matchesStateWithShift, s_keycodeForAKey, _, 2))
            .Times(1);

        EXPECT_CALL(m_interface, xkb_state_key_get_utf8(&m_xkbState, s_keycodeForShiftLKey, nullptr, 0))
            .Times(1);

        InputTextNotificationListener textListener;
        EXPECT_CALL(textListener, OnInputTextEvent(StrEq("a"), _)).Times(1);
        EXPECT_CALL(textListener, OnInputTextEvent(StrEq("A"), _)).Times(1);

        m_application.Start();

        for (int i = 0; i < 4; ++i)
        {
            m_application.PumpSystemEventLoopUntilEmpty();
            m_application.TickSystem();
            m_application.Tick();
        }
    }
} // namespace AzFramework
