/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <xcb/xcb.h>

#include <AzFramework/XcbApplication.h>
#include <AzFramework/XcbInputDeviceKeyboard.h>
#include "MockXcbInterface.h"
#include "Actions.h"

template<typename T>
xcb_generic_event_t MakeEvent(T event)
{
    return *reinterpret_cast<xcb_generic_event_t*>(&event);
}

namespace AzFramework
{
    TEST(XcbInputDeviceKeyboard, InputChannelsUpdateStateFromXcbEvents)
    {
        using testing::Return;
        using testing::Eq;
        using testing::_;
        MockXcbInterface interface;

        xcb_connection_t connection{};
        xkb_context xkbContext{};
        xkb_keymap xkbKeymap{};
        xkb_state xkbState{};
        const int32_t coreDeviceId{1};

        constexpr xcb_keycode_t keycodeForAKey = 38;

        const AZStd::array events
        {
            MakeEvent(xcb_key_press_event_t{
                /*.response_type = */ XCB_KEY_PRESS,
                /*.detail        = */ keycodeForAKey,
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
                /*.detail        = */ keycodeForAKey,
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

        EXPECT_CALL(interface, xcb_connect(_, _))
            .WillOnce(Return(&connection));
        EXPECT_CALL(interface, xcb_disconnect(&connection))
            .Times(1);

        EXPECT_CALL(interface, xkb_context_new(XKB_CONTEXT_NO_FLAGS))
            .WillOnce(Return(&xkbContext));
        EXPECT_CALL(interface, xkb_context_unref(&xkbContext))
            .Times(1);

        EXPECT_CALL(interface, xkb_x11_keymap_new_from_device(&xkbContext, &connection, coreDeviceId, XKB_KEYMAP_COMPILE_NO_FLAGS))
            .WillOnce(Return(&xkbKeymap));
        EXPECT_CALL(interface, xkb_keymap_unref(&xkbKeymap))
            .Times(1);

        EXPECT_CALL(interface, xkb_x11_state_new_from_device(&xkbKeymap, &connection, coreDeviceId))
            .WillOnce(Return(&xkbState));
        EXPECT_CALL(interface, xkb_state_unref(&xkbState))
            .Times(1);

        EXPECT_CALL(interface, xcb_xkb_use_extension(&connection, 1, 0));
        EXPECT_CALL(interface, xcb_xkb_use_extension_reply(&connection, _, _))
            .WillOnce(ReturnMalloc<xcb_xkb_use_extension_reply_t>(
                /* .response_type =*/static_cast<uint8_t>(XCB_XKB_USE_EXTENSION),
                /* .supported =*/ static_cast<uint8_t>(1))
            );
        EXPECT_CALL(interface, xkb_x11_get_core_keyboard_device_id(&connection))
            .WillRepeatedly(Return(coreDeviceId));

        // Set the expectations for the events that will be generated
        // nullptr entries represent when the event queue is empty, and will cause
        // PumpSystemEventLoopUntilEmpty to return
        // event pointers are freed by the calling code, so we malloc new copies
        // here
        EXPECT_CALL(interface, xcb_poll_for_event(&connection))
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[0]))
            .WillOnce(Return(nullptr))
            .WillOnce(ReturnMalloc<xcb_generic_event_t>(events[1]))
            .WillOnce(Return(nullptr))
            ;

        EXPECT_CALL(interface, xkb_state_key_get_one_sym(&xkbState, keycodeForAKey))
            .WillOnce(Return(XKB_KEY_a))
            .WillOnce(Return(XKB_KEY_a))
            ;

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
