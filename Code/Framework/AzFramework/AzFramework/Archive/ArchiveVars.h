/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzFramework/Archive/ArchiveVars_Platform.h>

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
        int nReadSlice{};
        int nSaveTotalResourceList{};
        int nSaveFastloadResourceList{};
        int nSaveMenuCommonResourceList{};
        int nSaveLevelResourceList{};
        int nValidateFileHashes{ IsReleaseConfig ? 0 : 1 };
        int nUncachedStreamReads{ 1 };
        int nInMemoryPerPakSizeLimit{ 6 }; // Limits in MB
        int nTotalInMemoryPakSizeLimit{ 30 };
        int nLoadCache{};
        int nLoadModePaks{};
        int nStreamCache{ STREAM_CACHE_DEFAULT };
        FileSearchPriority m_fileSearchPriority{ GetDefaultFileSearchPriority()};
        int nMessageInvalidFileAccess{};
        int nLogInvalidFileAccess{ IsReleaseConfig ? 0 : 1 };
        int nDisableNonLevelRelatedPaks{ 1 };
        int nWarnOnPakAccessFails{ 1 }; // Whether to treat failed pak access as a warning or log message
        int nSetLogLevel{ 3 };
        int nLogAllFileAccess{};

    };
}
