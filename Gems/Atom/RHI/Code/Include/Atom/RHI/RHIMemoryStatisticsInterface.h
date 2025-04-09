/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Budget.h>
#include <Atom/RHI.Reflect/MemoryStatistics.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

AZ_DECLARE_BUDGET(RHI);

AZ_CVAR_EXTERNED(bool, r_EnableAutoGpuMemDump); 

namespace AZ::RHI
{
    // Forward declares
    struct TransientAttachmentStatistics;
    class RHIMemoryStatisticsInterface
    {
    public:
        AZ_RTTI(RHIMemoryStatisticsInterface, "{C3789EE2-7922-434D-AC19-8A2D80194C0E}");

        static RHIMemoryStatisticsInterface* Get();

        RHIMemoryStatisticsInterface() = default;
        virtual ~RHIMemoryStatisticsInterface() = default;

        // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
        AZ_DISABLE_COPY_MOVE(RHIMemoryStatisticsInterface);

        virtual AZStd::unordered_map<int, TransientAttachmentStatistics> GetTransientAttachmentStatistics() const = 0;

        virtual const RHI::MemoryStatistics* GetMemoryStatistics() const = 0;

        //! Utility function to write the state of the provided pool statistics to json.
        //! The function assumes the pool data will not be modified while it is being converted.
        virtual void WriteResourcePoolInfoToJson(
            const AZStd::vector<RHI::MemoryStatistics::Pool>& pools, 
            rapidjson::Document& doc) const = 0;

        //! Utility function to load previously captured pool statistics from json.
        //! The function clears the passed pools and heaps vectors.
        virtual AZ::Outcome<void, AZStd::string> LoadResourcePoolInfoFromJson(
            AZStd::vector<RHI::MemoryStatistics::Pool>& pools, 
            AZStd::vector<RHI::MemoryStatistics::Heap>& heaps, 
            rapidjson::Document& doc, 
            const AZStd::string& fileName) const = 0;

        //! Utility function to write the current state of all resource pools to a json file
        //! Useful for programmatically triggered dumps.
        virtual void TriggerResourcePoolAllocInfoDump() const = 0;
    };
}

#ifndef AZ_RELEASE_BUILD
    #define AZ_RHI_DUMP_POOL_INFO_ON_FAIL(result) \
    do \
    { \
        if (r_EnableAutoGpuMemDump && !(result)) \
        { \
            AZ::RHI::RHIMemoryStatisticsInterface::Get()->TriggerResourcePoolAllocInfoDump(); \
        } \
    } while(false)
#else
    #define AZ_RHI_DUMP_POOL_INFO_ON_FAIL(result)
#endif

