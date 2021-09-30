/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <xcb/xcb.h>

#define explicit ExplicitIsACXXKeyword
#include <xcb/xkb.h>
#undef explicit
#include <xkbcommon/xkbcommon.h>

#include "Printers.h"

// xcb / xkb do not provide definitions of these structs, but the tests need some concrete value for them.
struct xcb_connection_t
{
};

struct xkb_context
{
};

struct xkb_keymap
{
};

struct xkb_state
{
};

class MockXcbInterface
{
public:
    MockXcbInterface()
    {
        self = this;
    }
    MockXcbInterface(const MockXcbInterface&) = delete;
    MockXcbInterface(MockXcbInterface&&) = delete;
    MockXcbInterface& operator=(const MockXcbInterface&) = delete;
    MockXcbInterface& operator=(MockXcbInterface&&) = delete;
    ~MockXcbInterface()
    {
        self = nullptr;
    }

    static MockXcbInterface* Instance() { return self; }

    // xcb
    MOCK_CONST_METHOD2(xcb_connect, xcb_connection_t*(const char* displayname, int* screenp));
    MOCK_CONST_METHOD1(xcb_disconnect, void(xcb_connection_t* c));
    MOCK_CONST_METHOD1(xcb_poll_for_event, xcb_generic_event_t*(xcb_connection_t* c));

    // xcb-xkb
    MOCK_CONST_METHOD3(xcb_xkb_use_extension, xcb_xkb_use_extension_cookie_t(xcb_connection_t* c, uint16_t wantedMajor, uint16_t wantedMinor));
    MOCK_CONST_METHOD3(xcb_xkb_use_extension_reply, xcb_xkb_use_extension_reply_t*(xcb_connection_t* c, xcb_xkb_use_extension_cookie_t cookie, xcb_generic_error_t** e));

    // xkb-x11
    MOCK_CONST_METHOD1(xkb_x11_get_core_keyboard_device_id, int32_t(xcb_connection_t* connection));
    MOCK_CONST_METHOD4(xkb_x11_keymap_new_from_device, xkb_keymap*(xkb_context* context, xcb_connection_t* connection, int32_t device_id, xkb_keymap_compile_flags flags));
    MOCK_CONST_METHOD3(xkb_x11_state_new_from_device, xkb_state*(xkb_keymap* keymap, xcb_connection_t* connection, int32_t device_id));

    // xkbcommon
    MOCK_CONST_METHOD1(xkb_context_new, xkb_context*(xkb_context_flags flags));
    MOCK_CONST_METHOD1(xkb_context_unref, void(xkb_context* context));
    MOCK_CONST_METHOD1(xkb_keymap_unref, void(xkb_keymap* keymap));
    MOCK_CONST_METHOD1(xkb_state_unref, void(xkb_state* state));
    MOCK_CONST_METHOD2(xkb_state_key_get_one_sym, xkb_keysym_t(xkb_state *state, xkb_keycode_t key));

private:
    static inline MockXcbInterface* self = nullptr;
};
