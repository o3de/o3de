/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>

namespace ScriptAutomation
{
    namespace Utils
    {
        //! Query if the system supports resizing the renderable area of the default window
        bool SupportsResizeClientAreaOfDefaultWindow();
        
        //! Resize the renderable area of the default window
        void ResizeClientArea(uint32_t width, uint32_t height);

        bool SupportsToggleFullScreenOfDefaultWindow();
        void ToggleFullScreenOfDefaultWindow();

        //! retrieve the default script screenshots output folder
        AZStd::string GetScreenshotsPath(bool resolvePath);

        //! retrieve the default script profiling data output folder
        AZStd::string GetProfilingPath(bool resolvePath);

        //! Provides a more convenient way to call AZ::IO::FileIOBase::GetInstance()->ResolvePath()
        AZStd::string ResolvePath(const AZStd::string& path);

        //! Returns true if the file resides within a folder
        bool IsFileUnderFolder(AZStd::string filePath, AZStd::string folder);

    } // namespace Utils
} // namespace ScriptAutomation
