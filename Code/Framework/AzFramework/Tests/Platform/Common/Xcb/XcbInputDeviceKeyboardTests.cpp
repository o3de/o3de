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
#include "MockXcbInterface.h"
#include "Actions.h"
#include "XcbBaseTestFixture.h"

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

            constexpr unsigned int xcbXkbSelectEventsSequence = 1;
            ON_CALL(m_interface, xcb_xkb_select_events(&m_connection, _, _, _, _, _, _, _))
                .WillByDefault(Return(xcb_void_cookie_t{/*.sequence = */ xcbXkbSelectEventsSequence}));
            ON_CALL(m_interface, xcb_request_check(&m_connection, testing::Field(&xcb_void_cookie_t::sequence, testing::Eq(xcbXkbSelectEventsSequence))))
                .WillByDefault(Return(nullptr)); // indicates success

            ON_CALL(m_interface, xkb_x11_get_core_keyboard_device_id(&m_connection))
                .WillByDefault(Return(s_coreDeviceId));

            ON_CALL(m_interface, xkb_state_key_get_one_sym(&m_xkbState, s_keycodeForAKey))
                .WillByDefault(Return(XKB_KEY_a))
                ;
        }

    protected:
        xkb_context m_xkbContext{};
        xkb_keymap m_xkbKeymap{};
        xkb_state m_xkbState{};
        static constexpr int32_t s_coreDeviceId{1};
        static constexpr uint8_t s_xkbEventCode{85};
        static constexpr xcb_keycode_t s_keycodeForAKey{38};
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

        EXPECT_CALL(m_interface, xkb_state_key_get_utf8(&m_xkbState, s_keycodeForAKey, nullptr, 0))
            .WillRepeatedly(Return(1));
        EXPECT_CALL(m_interface, xkb_state_key_get_utf8(&m_xkbState, s_keycodeForAKey, _, 2))
            .WillRepeatedly(DoAll(
                SetArgPointee<2>('a'),
                Return(1)
            ));

        Application application;
        application.Start({}, {});

        const InputChannel* inputChannel = InputChannelRequests::FindInputChannel(InputDeviceKeyboard::Key::AlphanumericA);
        ASSERT_TRUE(inputChannel);
        EXPECT_THAT(inputChannel->GetState(), Eq(InputChannel::State::Idle));

        application.PumpSystemEventLoopUntilEmpty();
        application.TickSystem();
        application.Tick();

        EXPECT_THAT(inputChannel->GetState(), Eq(InputChannel::State::Began));

        application.PumpSystemEventLoopUntilEmpty();
        application.TickSystem();
        application.Tick();

        EXPECT_THAT(inputChannel->GetState(), Eq(InputChannel::State::Ended));

        application.Stop();
    }
} // namespace AzFramework
