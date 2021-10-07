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

// ----------------------------------------------------------------------------
// xkb-x11
int32_t xkb_x11_get_core_keyboard_device_id(xcb_connection_t* connection)
{
    return MockXcbInterface::Instance()->xkb_x11_get_core_keyboard_device_id(connection);
}
struct xkb_keymap* xkb_x11_keymap_new_from_device(struct xkb_context* context, xcb_connection_t* connection, int32_t device_id, enum xkb_keymap_compile_flags flags)
{
    return MockXcbInterface::Instance()->xkb_x11_keymap_new_from_device(context, connection, device_id, flags);
}
xkb_state* xkb_x11_state_new_from_device(xkb_keymap* keymap, xcb_connection_t* connection, int32_t device_id)
{
    return MockXcbInterface::Instance()->xkb_x11_state_new_from_device(keymap, connection, device_id);
}

// ----------------------------------------------------------------------------
// xkbcommon
xkb_context* xkb_context_new(enum xkb_context_flags flags)
{
    return MockXcbInterface::Instance()->xkb_context_new(flags);
}
void xkb_context_unref(xkb_context *context)
{
    return MockXcbInterface::Instance()->xkb_context_unref(context);
}
void xkb_keymap_unref(xkb_keymap *keymap)
{
    return MockXcbInterface::Instance()->xkb_keymap_unref(keymap);
}
void xkb_state_unref(xkb_state *state)
{
    return MockXcbInterface::Instance()->xkb_state_unref(state);
}
xkb_keysym_t xkb_state_key_get_one_sym(xkb_state *state, xkb_keycode_t key)
{
    return MockXcbInterface::Instance()->xkb_state_key_get_one_sym(state, key);
}

}
