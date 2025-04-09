/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/AzFrameworkStatic.h>
#include <AzFramework/Archive/ArchiveVars.h>
#include <AzCore/Console/IConsoleTypes.h>
namespace AZ::IO
{
    AZ_CVAR(int, sys_PakPriority, aznumeric_cast<int>(ArchiveVars{}.m_fileSearchPriority), nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "If set to 0, tells Archive to try to open the file on the file system first otherwise check mounted paks.\n"
        "If set to 1, tells Archive to try to open the file in pak first, then go to file system.\n"
        "If set to 2, tells the Archive to only open files from the pak");
    AZ_CVAR(int, sys_report_files_not_found_in_paks, 0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Reports when files are searched for in paks and not found. 1 = log, 2 = warning, 3 = error");
    AZ_CVAR(int32_t, az_archive_verbosity, 0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Sets the verbosity level for logging Archive operations\n"
        ">=1 - Turns on verbose logging of all operations");
} // namespace AZ::IO

namespace AzFramework
{
    AZ_CVAR(bool, ed_cameraSystemUseCursor, true, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Should the camera use cursor absolute positions or motion deltas");
} // namespace AzFramework

