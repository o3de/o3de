/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define AZ_TRAIT_AZFRAMEWORK_AWS_ENABLE_TCP_KEEP_ALIVE_SUPPORTED (false)
#define AZ_TRAIT_AZFRAMEWORK_BOOTSTRAP_CFG_CURRENT_PLATFORM "windows"
#define AZ_TRAIT_AZFRAMEWORK_USE_C11_LOCALTIME_S 1
#define AZ_TRAIT_AZFRAMEWORK_USE_ERRNO_T_TYPEDEF 0
#define AZ_TRAIT_AZFRAMEWORK_USE_POSIX_LOCALTIME_R 0
#define AZ_TRAIT_AZFRAMEWORK_PYTHON_SHELL "python.cmd"
#define AZ_TRAIT_AZFRAMEWORK_USE_PROJECT_MANAGER 1
#define AZ_TRAIT_AZFRAMEWORK_PROCESSLAUNCH_DEFAULT 0

// On Linux is necessary, on Windows is not necessary to display
// the mouse when Lua hits a breakpoint. On Windows, for the most part
// it is harmless, but there's an annoying effect if enabling
// this trait because it causes the mouse pointer to reposition
// itself close to the client area of the render viewport.
// To avoid this minor annoyance we set this to 0 on windows.
#define AZ_TRAIT_AZFRAMEWORK_SHOW_MOUSE_ON_LUA_BREAKPOINT 0 
