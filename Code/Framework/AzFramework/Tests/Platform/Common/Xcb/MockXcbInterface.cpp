/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MockXcbInterface.h"

// The functions defined in this file will take precedence over those defined
// in the real libxcb.so, allowing the test code to use a mock implementation

extern "C"
{

// ----------------------------------------------------------------------------
// xcb
xcb_connection_t* xcb_connect(const char* displayname, int* screenp)
{
    return MockXcbInterface::Instance()->xcb_connect(displayname, screenp);
}
void xcb_disconnect(xcb_connection_t* c)
{
    return MockXcbInterface::Instance()->xcb_disconnect(c);
}
xcb_generic_event_t* xcb_poll_for_event(xcb_connection_t* c)
{
    return MockXcbInterface::Instance()->xcb_poll_for_event(c);
}
xcb_generic_error_t* xcb_request_check(xcb_connection_t* c, xcb_void_cookie_t cookie)
{
    return MockXcbInterface::Instance()->xcb_request_check(c, cookie);
}

// ----------------------------------------------------------------------------
// xcb-xkb
xcb_xkb_use_extension_cookie_t xcb_xkb_use_extension(xcb_connection_t* c, uint16_t wantedMajor, uint16_t wantedMinor)
{
    return MockXcbInterface::Instance()->xcb_xkb_use_extension(c, wantedMajor, wantedMinor);
}
xcb_xkb_use_extension_reply_t* xcb_xkb_use_extension_reply(xcb_connection_t* c, xcb_xkb_use_extension_cookie_t cookie, xcb_generic_error_t** e)
{
    return MockXcbInterface::Instance()->xcb_xkb_use_extension_reply(c, cookie, e);
}
xcb_void_cookie_t xcb_xkb_select_events(xcb_connection_t* c, xcb_xkb_device_spec_t deviceSpec, uint16_t affectWhich, uint16_t clear, uint16_t selectAll, uint16_t affectMap, uint16_t map, const void* details)
{
    return MockXcbInterface::Instance()->xcb_xkb_select_events(c, deviceSpec, affectWhich, clear, selectAll, affectMap, map, details);
}

// ----------------------------------------------------------------------------
// xkb-x11
int32_t xkb_x11_get_core_keyboard_device_id(xcb_connection_t* connection)
{
    return MockXcbInterface::Instance()->xkb_x11_get_core_keyboard_device_id(connection);
}
xkb_keymap* xkb_x11_keymap_new_from_device(xkb_context* context, xcb_connection_t* connection, int32_t device_id, xkb_keymap_compile_flags flags)
{
    return MockXcbInterface::Instance()->xkb_x11_keymap_new_from_device(context, connection, device_id, flags);
}
xkb_state* xkb_x11_state_new_from_device(xkb_keymap* keymap, xcb_connection_t* connection, int32_t device_id)
{
    return MockXcbInterface::Instance()->xkb_x11_state_new_from_device(keymap, connection, device_id);
}
int xkb_x11_setup_xkb_extension(
    xcb_connection_t* connection,
    uint16_t major_xkb_version,
    uint16_t minor_xkb_version,
    xkb_x11_setup_xkb_extension_flags flags,
    uint16_t* major_xkb_version_out,
    uint16_t* minor_xkb_version_out,
    uint8_t* base_event_out,
    uint8_t* base_error_out)
{
    return MockXcbInterface::Instance()->xkb_x11_setup_xkb_extension(
        connection, major_xkb_version, minor_xkb_version, flags, major_xkb_version_out, minor_xkb_version_out, base_event_out,
        base_error_out);
}

// ----------------------------------------------------------------------------
// xkbcommon
xkb_context* xkb_context_new(xkb_context_flags flags)
{
    return MockXcbInterface::Instance()->xkb_context_new(flags);
}
void xkb_context_unref(xkb_context* context)
{
    return MockXcbInterface::Instance()->xkb_context_unref(context);
}
void xkb_keymap_unref(xkb_keymap* keymap)
{
    return MockXcbInterface::Instance()->xkb_keymap_unref(keymap);
}
void xkb_state_unref(xkb_state* state)
{
    return MockXcbInterface::Instance()->xkb_state_unref(state);
}
xkb_keysym_t xkb_state_key_get_one_sym(xkb_state* state, xkb_keycode_t key)
{
    return MockXcbInterface::Instance()->xkb_state_key_get_one_sym(state, key);
}
int xkb_state_key_get_utf8(xkb_state* state, xkb_keycode_t key, char* buffer, size_t size)
{
    return MockXcbInterface::Instance()->xkb_state_key_get_utf8(state, key, buffer, size);
}
xkb_state_component xkb_state_update_mask(
    xkb_state* state,
    xkb_mod_mask_t depressed_mods,
    xkb_mod_mask_t latched_mods,
    xkb_mod_mask_t locked_mods,
    xkb_layout_index_t depressed_layout,
    xkb_layout_index_t latched_layout,
    xkb_layout_index_t locked_layout)
{
    return MockXcbInterface::Instance()->xkb_state_update_mask(
        state, depressed_mods, latched_mods, locked_mods, depressed_layout, latched_layout, locked_layout);
}

}
