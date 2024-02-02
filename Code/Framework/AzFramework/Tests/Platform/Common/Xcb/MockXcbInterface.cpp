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
const xcb_setup_t* xcb_get_setup(xcb_connection_t *c)
{
    return MockXcbInterface::Instance()->xcb_get_setup(c);
}
xcb_screen_iterator_t xcb_setup_roots_iterator(const xcb_setup_t* R)
{
    return MockXcbInterface::Instance()->xcb_setup_roots_iterator(R);
}
const xcb_query_extension_reply_t* xcb_get_extension_data(xcb_connection_t* c, xcb_extension_t* ext)
{
    return MockXcbInterface::Instance()->xcb_get_extension_data(c, ext);
}
int xcb_flush(xcb_connection_t *c)
{
    return MockXcbInterface::Instance()->xcb_flush(c);
}
xcb_query_pointer_cookie_t xcb_query_pointer(xcb_connection_t* c, xcb_window_t window)
{
    return MockXcbInterface::Instance()->xcb_query_pointer(c, window);
}
xcb_query_pointer_reply_t* xcb_query_pointer_reply(xcb_connection_t* c, xcb_query_pointer_cookie_t cookie, xcb_generic_error_t** e)
{
    return MockXcbInterface::Instance()->xcb_query_pointer_reply(c, cookie, e);
}
xcb_get_geometry_cookie_t xcb_get_geometry(xcb_connection_t* c, xcb_drawable_t drawable)
{
    return MockXcbInterface::Instance()->xcb_get_geometry(c, drawable);
}
xcb_get_geometry_reply_t* xcb_get_geometry_reply(xcb_connection_t* c, xcb_get_geometry_cookie_t cookie, xcb_generic_error_t** e)
{
    return MockXcbInterface::Instance()->xcb_get_geometry_reply(c, cookie, e);
}
xcb_void_cookie_t xcb_warp_pointer(
    xcb_connection_t* c,
    xcb_window_t src_window,
    xcb_window_t dst_window,
    int16_t src_x,
    int16_t src_y,
    uint16_t src_width,
    uint16_t src_height,
    int16_t dst_x,
    int16_t dst_y)
{
    return MockXcbInterface::Instance()->xcb_warp_pointer(c, src_window, dst_window, src_x, src_y, src_width, src_height, dst_x, dst_y);
}
xcb_intern_atom_cookie_t xcb_intern_atom(xcb_connection_t* c, uint8_t only_if_exists, uint16_t name_len, const char* name)
{
    return MockXcbInterface::Instance()->xcb_intern_atom(c, only_if_exists, name_len, name);
}
xcb_intern_atom_reply_t* xcb_intern_atom_reply(xcb_connection_t* c, xcb_intern_atom_cookie_t cookie, xcb_generic_error_t** e)
{
    return MockXcbInterface::Instance()->xcb_intern_atom_reply(c, cookie, e);
}
xcb_get_property_cookie_t xcb_get_property(
    xcb_connection_t* c,
    uint8_t _delete,
    xcb_window_t window,
    xcb_atom_t property,
    xcb_atom_t type,
    uint32_t long_offset,
    uint32_t long_length)
{
    return MockXcbInterface::Instance()->xcb_get_property(c, _delete, window, property, type, long_offset, long_length);
}
xcb_get_property_reply_t* xcb_get_property_reply(xcb_connection_t* c, xcb_get_property_cookie_t cookie, xcb_generic_error_t** e)
{
    return MockXcbInterface::Instance()->xcb_get_property_reply(c, cookie, e);
}
void* xcb_get_property_value(const xcb_get_property_reply_t* R)
{
    return MockXcbInterface::Instance()->xcb_get_property_value(R);
}
uint32_t xcb_generate_id(xcb_connection_t *c)
{
    return MockXcbInterface::Instance()->xcb_generate_id(c);
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

// ----------------------------------------------------------------------------
// xcb-xfixes
xcb_xfixes_query_version_cookie_t xcb_xfixes_query_version(
    xcb_connection_t* c, uint32_t client_major_version, uint32_t client_minor_version)
{
    return MockXcbInterface::Instance()->xcb_xfixes_query_version(c, client_major_version, client_minor_version);
}
xcb_xfixes_query_version_reply_t* xcb_xfixes_query_version_reply(
    xcb_connection_t* c, xcb_xfixes_query_version_cookie_t cookie, xcb_generic_error_t** e)
{
    return MockXcbInterface::Instance()->xcb_xfixes_query_version_reply(c, cookie, e);
}
xcb_void_cookie_t xcb_xfixes_show_cursor_checked(xcb_connection_t* c, xcb_window_t window)
{
    return MockXcbInterface::Instance()->xcb_xfixes_show_cursor_checked(c, window);
}
xcb_void_cookie_t xcb_xfixes_hide_cursor_checked(xcb_connection_t* c, xcb_window_t window)
{
    return MockXcbInterface::Instance()->xcb_xfixes_hide_cursor_checked(c, window);
}
xcb_void_cookie_t xcb_xfixes_delete_pointer_barrier_checked(xcb_connection_t* c, xcb_xfixes_barrier_t barrier)
{
    return MockXcbInterface::Instance()->xcb_xfixes_delete_pointer_barrier_checked(c, barrier);
}
xcb_translate_coordinates_cookie_t xcb_translate_coordinates(xcb_connection_t* c, xcb_window_t src_window, xcb_window_t dst_window, int16_t src_x, int16_t src_y)
{
    return MockXcbInterface::Instance()->xcb_translate_coordinates(c, src_window, dst_window, src_x, src_y);
}
xcb_translate_coordinates_reply_t* xcb_translate_coordinates_reply(xcb_connection_t* c, xcb_translate_coordinates_cookie_t cookie, xcb_generic_error_t** e)
{
    return MockXcbInterface::Instance()->xcb_translate_coordinates_reply(c, cookie, e);
}
xcb_void_cookie_t xcb_xfixes_create_pointer_barrier_checked(
    xcb_connection_t* c,
    xcb_xfixes_barrier_t barrier,
    xcb_window_t window,
    uint16_t x1,
    uint16_t y1,
    uint16_t x2,
    uint16_t y2,
    uint32_t directions,
    uint16_t num_devices,
    const uint16_t* devices)
{
    return MockXcbInterface::Instance()->xcb_xfixes_create_pointer_barrier_checked(c, barrier, window, x1, y1, x2, y2, directions, num_devices, devices);
}

// ----------------------------------------------------------------------------
// xcb-xinput
xcb_input_xi_query_version_cookie_t xcb_input_xi_query_version(xcb_connection_t* c, uint16_t major_version, uint16_t minor_version)
{
    return MockXcbInterface::Instance()->xcb_input_xi_query_version(c, major_version, minor_version);
}
xcb_input_xi_query_version_reply_t* xcb_input_xi_query_version_reply(
    xcb_connection_t* c, xcb_input_xi_query_version_cookie_t cookie, xcb_generic_error_t** e)
{
    return MockXcbInterface::Instance()->xcb_input_xi_query_version_reply(c, cookie, e);
}
xcb_void_cookie_t xcb_input_xi_select_events(
    xcb_connection_t* c, xcb_window_t window, uint16_t num_mask, const xcb_input_event_mask_t* masks)
{
    return MockXcbInterface::Instance()->xcb_input_xi_select_events(c, window, num_mask, masks);
}
int xcb_input_raw_button_press_axisvalues_length (const xcb_input_raw_button_press_event_t *R)
{
    return MockXcbInterface::Instance()->xcb_input_raw_button_press_axisvalues_length(R);
}
xcb_input_fp3232_t* xcb_input_raw_button_press_axisvalues_raw(const xcb_input_raw_button_press_event_t* R)
{
    return MockXcbInterface::Instance()->xcb_input_raw_button_press_axisvalues_raw(R);
}
uint32_t* xcb_input_raw_button_press_valuator_mask (const xcb_input_raw_button_press_event_t *R)
{
    return MockXcbInterface::Instance()->xcb_input_raw_button_press_valuator_mask(R);
}
}
