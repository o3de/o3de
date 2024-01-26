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
#include <xcb/xfixes.h>
#include <xcb/xinput.h>

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
    MOCK_CONST_METHOD1(xcb_get_setup, const xcb_setup_t*(xcb_connection_t *c));
    MOCK_CONST_METHOD1(xcb_setup_roots_iterator, xcb_screen_iterator_t(const xcb_setup_t* R));
    MOCK_CONST_METHOD2(xcb_get_extension_data, const xcb_query_extension_reply_t*(xcb_connection_t* c, xcb_extension_t* ext));
    MOCK_CONST_METHOD1(xcb_flush, int(xcb_connection_t *c));
    MOCK_CONST_METHOD2(xcb_query_pointer, xcb_query_pointer_cookie_t(xcb_connection_t* c, xcb_window_t window));
    MOCK_CONST_METHOD3(xcb_query_pointer_reply, xcb_query_pointer_reply_t*(xcb_connection_t* c, xcb_query_pointer_cookie_t cookie, xcb_generic_error_t** e));
    MOCK_CONST_METHOD2(xcb_get_geometry, xcb_get_geometry_cookie_t(xcb_connection_t* c, xcb_drawable_t drawable));
    MOCK_CONST_METHOD3(xcb_get_geometry_reply, xcb_get_geometry_reply_t*(xcb_connection_t* c, xcb_get_geometry_cookie_t cookie, xcb_generic_error_t** e));
    MOCK_CONST_METHOD9(xcb_warp_pointer, xcb_void_cookie_t(
        xcb_connection_t* c,
        xcb_window_t src_window,
        xcb_window_t dst_window,
        int16_t src_x,
        int16_t src_y,
        uint16_t src_width,
        uint16_t src_height,
        int16_t dst_x,
        int16_t dst_y));
    MOCK_CONST_METHOD4(xcb_intern_atom, xcb_intern_atom_cookie_t(xcb_connection_t* c, uint8_t only_if_exists, uint16_t name_len, const char* name));
    MOCK_CONST_METHOD3(xcb_intern_atom_reply, xcb_intern_atom_reply_t*(xcb_connection_t* c, xcb_intern_atom_cookie_t cookie, xcb_generic_error_t** e));
    MOCK_CONST_METHOD7(xcb_get_property, xcb_get_property_cookie_t(
        xcb_connection_t* c,
        uint8_t _delete,
        xcb_window_t window,
        xcb_atom_t property,
        xcb_atom_t type,
        uint32_t long_offset,
        uint32_t long_length));
    MOCK_CONST_METHOD3(xcb_get_property_reply, xcb_get_property_reply_t*(xcb_connection_t* c, xcb_get_property_cookie_t cookie, xcb_generic_error_t** e));
    MOCK_CONST_METHOD1(xcb_get_property_value, void*(const xcb_get_property_reply_t* R));
    MOCK_CONST_METHOD1(xcb_generate_id, uint32_t(xcb_connection_t *c));

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

    // xcb-xfixes
    MOCK_CONST_METHOD3(xcb_xfixes_query_version, xcb_xfixes_query_version_cookie_t(xcb_connection_t* c, uint32_t client_major_version, uint32_t client_minor_version));
    MOCK_CONST_METHOD3(xcb_xfixes_query_version_reply, xcb_xfixes_query_version_reply_t*(xcb_connection_t* c, xcb_xfixes_query_version_cookie_t cookie, xcb_generic_error_t** e));
    MOCK_CONST_METHOD2(xcb_xfixes_show_cursor_checked, xcb_void_cookie_t(xcb_connection_t* c, xcb_window_t window));
    MOCK_CONST_METHOD2(xcb_xfixes_hide_cursor_checked, xcb_void_cookie_t(xcb_connection_t* c, xcb_window_t window));
    MOCK_CONST_METHOD2(xcb_xfixes_delete_pointer_barrier_checked, xcb_void_cookie_t(xcb_connection_t* c, xcb_xfixes_barrier_t barrier));
    MOCK_CONST_METHOD5(xcb_translate_coordinates, xcb_translate_coordinates_cookie_t(xcb_connection_t* c, xcb_window_t src_window, xcb_window_t dst_window, int16_t src_x, int16_t src_y));
    MOCK_CONST_METHOD3(xcb_translate_coordinates_reply, xcb_translate_coordinates_reply_t*(xcb_connection_t* c, xcb_translate_coordinates_cookie_t cookie, xcb_generic_error_t** e));
    MOCK_CONST_METHOD10(xcb_xfixes_create_pointer_barrier_checked, xcb_void_cookie_t(
        xcb_connection_t* c,
        xcb_xfixes_barrier_t barrier,
        xcb_window_t window,
        uint16_t x1,
        uint16_t y1,
        uint16_t x2,
        uint16_t y2,
        uint32_t directions,
        uint16_t num_devices,
        const uint16_t* devices));

    // xcb-xinput
    MOCK_CONST_METHOD3(xcb_input_xi_query_version, xcb_input_xi_query_version_cookie_t(xcb_connection_t* c, uint16_t major_version, uint16_t minor_version));
    MOCK_CONST_METHOD3(xcb_input_xi_query_version_reply, xcb_input_xi_query_version_reply_t*(xcb_connection_t* c, xcb_input_xi_query_version_cookie_t cookie, xcb_generic_error_t** e));
    MOCK_CONST_METHOD4(xcb_input_xi_select_events, xcb_void_cookie_t(xcb_connection_t* c, xcb_window_t window, uint16_t num_mask, const xcb_input_event_mask_t* masks));
    MOCK_CONST_METHOD1(xcb_input_raw_button_press_axisvalues_length, int(const xcb_input_raw_button_press_event_t* R));
    MOCK_CONST_METHOD1(xcb_input_raw_button_press_axisvalues_raw, xcb_input_fp3232_t*(const xcb_input_raw_button_press_event_t* R));
    MOCK_CONST_METHOD1(xcb_input_raw_button_press_valuator_mask, uint32_t*(const xcb_input_raw_button_press_event_t* R));

private:
    static inline MockXcbInterface* self = nullptr;
};
