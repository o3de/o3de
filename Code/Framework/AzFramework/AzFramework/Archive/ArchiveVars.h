/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

namespace AZ::IO
{
    enum class FileSearchPriority
    {
        FileFirst,
        PakFirst,
        PakOnly
    };


    //file location enum used in isFileExist to control where the archive system looks for the file.
    enum class FileSearchLocation
    {
        Any,
        OnDisk,
        InPak
    };

    FileSearchPriority GetDefaultFileSearchPriority();

    // variables that control behavior of the Archive subsystem
    struct ArchiveVars
    {
#if defined(_RELEASE)
        inline static constexpr bool IsReleaseConfig{ true };
#else
        inline static constexpr bool IsReleaseConfig{};
#endif
        int nSaveLevelResourceList{};
        FileSearchPriority m_fileSearchPriority{ GetDefaultFileSearchPriority()};
        int nMessageInvalidFileAccess{};
        int nLogInvalidFileAccess{ IsReleaseConfig ? 0 : 1 };

    };
}
