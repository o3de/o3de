/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Module/DynamicModuleHandle.h>

// This header provides compatibility with the linux specific libevdev module, but without having to have the header
// present and without statically linking to it, by pulling in the shared library and finding the functions in it.

extern "C"
{
    // symbols are using a different name just in case the real library is already pulled in.
    const int O3DEWRAPPER_LIBEVDEV_READ_FLAG_NORMAL = 2;
    const int O3DEWRAPPER_LIBEVDEV_READ_STATUS_SUCCESS = 0;
    struct libevdev;
    struct input_absinfo;
    struct input_event;
}

// this class is meant to be created and destroyed in a shared_ptr so that it can be done once and passed down to many
// implementations.
namespace AzFramework
{
    // meant to be a purely public struct to expose the functions.
    struct LibEVDevWrapper
    {
        using functionType_libevdev_free                = void(*)(struct libevdev *dev);
        using functionType_libevdev_new_from_fd         = int(*)(int fd, struct libevdev **dev);
        using functionType_libevdev_has_event_code      = int(*)(struct libevdev *dev, unsigned int type, unsigned int code);
        using functionType_libevdev_get_name            = const char*(*)(struct libevdev *dev);
        using functionType_libevdev_event_code_get_name = const char*(*)(unsigned int type, unsigned int code);
        using functionType_libevdev_get_abs_info        = const struct input_absinfo*(*)(struct libevdev *dev, unsigned int code);
        using functionType_libevdev_next_event          = int(*)(struct libevdev *dev, unsigned int flags, struct input_event *ev);

        functionType_libevdev_free                  m_libevdev_free = nullptr;
        functionType_libevdev_new_from_fd           m_libevdev_new_from_fd = nullptr;
        functionType_libevdev_has_event_code        m_libevdev_has_event_code = nullptr;
        functionType_libevdev_get_name              m_libevdev_get_name = nullptr;
        functionType_libevdev_event_code_get_name   m_libevdev_event_code_get_name = nullptr;
        functionType_libevdev_get_abs_info          m_libevdev_get_abs_info = nullptr;
        functionType_libevdev_next_event            m_libevdev_next_event = nullptr;

        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_libevdevHandle;

        LibEVDevWrapper();
        ~LibEVDevWrapper();
    };
} // end namespace AzFramework
