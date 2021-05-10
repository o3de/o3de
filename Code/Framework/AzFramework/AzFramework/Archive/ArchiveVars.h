/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <AzFramework/Archive/ArchiveVars_Platform.h>

namespace AZ::IO
{
    enum class ArchiveLocationPriority
    {
        ePakPriorityFileFirst = 0,
        ePakPriorityPakFirst = 1,
        ePakPriorityPakOnly = 2
    };

    // variables that control behavior of Archive/StreamEngine subsystems
    struct ArchiveVars
    {
#if defined(_RELEASE)
        inline static constexpr bool IsReleaseConfig{ true };
#else
        inline static constexpr bool IsReleaseConfig{};
#endif

    public:
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
        ArchiveLocationPriority nPriority{ IsReleaseConfig
            ? ArchiveLocationPriority::ePakPriorityPakOnly
            : ArchiveLocationPriority::ePakPriorityFileFirst }; // Which file location to favor (loose vs. pak files)
        int nMessageInvalidFileAccess{};
        int nLogInvalidFileAccess{ IsReleaseConfig ? 0 : 1 };
        int nDisableNonLevelRelatedPaks{ 1 };
        int nWarnOnPakAccessFails{ 1 }; // Whether to treat failed pak access as a warning or log message
        int nSetLogLevel{ 3 };
        int nLogAllFileAccess{};

    };
}
