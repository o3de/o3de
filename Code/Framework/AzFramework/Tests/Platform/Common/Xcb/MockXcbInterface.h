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
#include <xkbcommon/xkbcommon-x11.h>

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
    xkb_mod_mask_t m_modifiers{};
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
    virtual ~MockXcbInterface()
    {
        self = nullptr;
    }

    static MockXcbInterface* Instance() { return self; }

    // xcb
    MOCK_CONST_METHOD2(xcb_connect, xcb_connection_t*(const char* displayname, int* screenp));
    MOCK_CONST_METHOD1(xcb_disconnect, void(xcb_connection_t* c));
    MOCK_CONST_METHOD1(xcb_poll_for_event, xcb_generic_event_t*(xcb_connection_t* c));
    MOCK_CONST_METHOD2(xcb_request_check, xcb_generic_error_t*(xcb_connection_t* c, xcb_void_cookie_t cookie));

    // xcb-xkb
    MOCK_CONST_METHOD3(xcb_xkb_use_extension, xcb_xkb_use_extension_cookie_t(xcb_connection_t* c, uint16_t wantedMajor, uint16_t wantedMinor));
    MOCK_CONST_METHOD3(xcb_xkb_use_extension_reply, xcb_xkb_use_extension_reply_t*(xcb_connection_t* c, xcb_xkb_use_extension_cookie_t cookie, xcb_generic_error_t** e));
    MOCK_CONST_METHOD8(xcb_xkb_select_events, xcb_void_cookie_t(xcb_connection_t* c, xcb_xkb_device_spec_t deviceSpec, uint16_t affectWhich, uint16_t clear, uint16_t selectAll, uint16_t affectMap, uint16_t map, const void* details));

    // xkb-x11
    MOCK_CONST_METHOD1(xkb_x11_get_core_keyboard_device_id, int32_t(xcb_connection_t* connection));
    MOCK_CONST_METHOD4(xkb_x11_keymap_new_from_device, xkb_keymap*(xkb_context* context, xcb_connection_t* connection, int32_t device_id, xkb_keymap_compile_flags flags));
    MOCK_CONST_METHOD3(xkb_x11_state_new_from_device, xkb_state*(xkb_keymap* keymap, xcb_connection_t* connection, int32_t device_id));
    MOCK_CONST_METHOD8(xkb_x11_setup_xkb_extension, int(xcb_connection_t* connection, uint16_t major_xkb_version, uint16_t minor_xkb_version, xkb_x11_setup_xkb_extension_flags flags, uint16_t* major_xkb_version_out, uint16_t* minor_xkb_version_out, uint8_t* base_event_out, uint8_t* base_error_out));

    // xkbcommon
    MOCK_CONST_METHOD1(xkb_context_new, xkb_context*(xkb_context_flags flags));
    MOCK_CONST_METHOD1(xkb_context_unref, void(xkb_context* context));
    MOCK_CONST_METHOD1(xkb_keymap_unref, void(xkb_keymap* keymap));
    MOCK_CONST_METHOD1(xkb_state_unref, void(xkb_state* state));
    MOCK_CONST_METHOD2(xkb_state_key_get_one_sym, xkb_keysym_t(xkb_state* state, xkb_keycode_t key));
    MOCK_CONST_METHOD4(xkb_state_key_get_utf8, int(xkb_state* state, xkb_keycode_t key, char* buffer, size_t size));
    MOCK_CONST_METHOD7(xkb_state_update_mask, xkb_state_component(xkb_state* state, xkb_mod_mask_t depressed_mods, xkb_mod_mask_t latched_mods, xkb_mod_mask_t locked_mods, xkb_layout_index_t depressed_layout, xkb_layout_index_t latched_layout, xkb_layout_index_t locked_layout));

private:
    static inline MockXcbInterface* self = nullptr;
};
