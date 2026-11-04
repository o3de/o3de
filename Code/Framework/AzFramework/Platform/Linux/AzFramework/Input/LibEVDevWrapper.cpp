/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzFramework/Input/LibEVDevWrapper.h>

namespace AzFramework
{
    LibEVDevWrapper::LibEVDevWrapper()
    {
        // Try the unversioned name first (present when the -dev/-devel package is
        // installed), then fall back to the versioned runtime SONAMEs that ship
        // with the shared library itself. Most end-user systems only have the
        // runtime package, which provides libevdev.so.2 but not the unversioned
        // libevdev.so symlink, so loading only "libevdev.so" silently disabled
        // gamepad support on an otherwise correctly configured machine. dlopen
        // resolves these SONAMEs through the dynamic linker cache.
        //
        // The versioned names are passed with correctModuleName = false: name
        // correction appends the platform library extension (".so") when the name
        // does not already end with it, which would turn "libevdev.so.2" into
        // "libevdev.so.2.so" and never resolve.
        struct LibraryCandidate
        {
            const char* m_name;
            bool m_correctModuleName;
        };
        constexpr LibraryCandidate candidates[] = {
            { "libevdev.so", true },
            { "libevdev.so.2", false },
            { "libevdev.so.1", false },
        };
        for (const LibraryCandidate& candidate : candidates)
        {
            auto handle = AZ::DynamicModuleHandle::Create(candidate.m_name, candidate.m_correctModuleName);
            if (handle && handle->Load())
            {
                m_libevdevHandle = AZStd::move(handle);
                break;
            }
        }

        if (!m_libevdevHandle)
        {
            AZ_Info("Input", "libevdev not found - gamepad support disabled.  Install libevdev to enable.\n");
            return;
        }

        m_libevdev_free = m_libevdevHandle->GetFunction<functionType_libevdev_free>("libevdev_free");
        m_libevdev_new_from_fd = m_libevdevHandle->GetFunction<functionType_libevdev_new_from_fd>("libevdev_new_from_fd");
        m_libevdev_has_event_code = m_libevdevHandle->GetFunction<functionType_libevdev_has_event_code>("libevdev_has_event_code");
        m_libevdev_get_name = m_libevdevHandle->GetFunction<functionType_libevdev_get_name>("libevdev_get_name");
        m_libevdev_event_code_get_name = m_libevdevHandle->GetFunction<functionType_libevdev_event_code_get_name>("libevdev_event_code_get_name");
        m_libevdev_get_abs_info = m_libevdevHandle->GetFunction<functionType_libevdev_get_abs_info>("libevdev_get_abs_info");
        m_libevdev_next_event = m_libevdevHandle->GetFunction<functionType_libevdev_next_event>("libevdev_next_event");

        if ((m_libevdev_free) &&
            (m_libevdev_new_from_fd) &&
            (m_libevdev_has_event_code) &&
            (m_libevdev_get_name) &&
            (m_libevdev_event_code_get_name) &&
            (m_libevdev_get_abs_info) &&
            (m_libevdev_next_event))
        {
            AZ_Trace("Input", "libevdev.so loaded and all symbols found.\n")
        }
        else
        {
            // this is not a normal circumstance, since libevdev.so should not be missing symbols
            // and its API is stable...
            AZ_Warning("Input", false, "libevdev.so loaded but missing symbols.\n");
            m_libevdevHandle.reset();
        }
    }

    LibEVDevWrapper::~LibEVDevWrapper()
    {
        if (m_libevdevHandle)
        {
            m_libevdevHandle.reset();
        }
    }

} // end namespace AzFramework
