/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// RHIUtils is for dumping common functionality that is used in several places across the RHI
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Outcome/Outcome.h>
 
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Format.h>

#include <Atom/RHI/Device.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    //! Gets a pointer to the RHI device from the RHI System
    Ptr<Device> GetRHIDevice();

    //! Gets the associated format capabilities for the provided attachment usage, access and type
    FormatCapabilities GetCapabilities(ScopeAttachmentUsage scopeUsage, AttachmentType attachmentType);
    FormatCapabilities GetCapabilities(ScopeAttachmentUsage scopeUsage, ScopeAttachmentAccess scopeAccess, AttachmentType attachmentType);

    //! Queries the RHI Device for the nearest supported format
    Format GetNearestDeviceSupportedFormat(Format requestedFormat);

    //! Checks the format against the list of supported formats and returns
    //! the nearest match, with a warning if the exact format is not supported
    Format ValidateFormat(Format format, const char* location, const AZStd::vector<Format>& formatFallbacks, FormatCapabilities requestedCapabilities = FormatCapabilities::None);

    //! Returns the command line value associated with the option if it exists.
    //! If multiple values exist it will return the last one
    AZStd::string GetCommandLineValue(const AZStd::string& commandLineOption);

    //! Returns true if the command line option is set
    bool QueryCommandLineOption(const AZStd::string& commandLineOption);

    //! Returns true if the current backend is null 
    bool IsNullRHI();

    //! Returns true if the Atom/GraphicsDevMode settings registry key is set
    bool IsGraphicsDevModeEnabled();

    //! Returns the default supervariant name of an empty string if float16 is supported and the name of "NoFloat16" if float16 is not
    //! supported. This is useful for loading the correct supervariant when a shader needs to have a version with and without float16.
    const AZ::Name& GetDefaultSupervariantNameWithNoFloat16Fallback();

    //! Utility function to write captured pool data to a json document
    //! Ensure the passed pool won't be modified during the call to this function
    //! Available externally to the RHI through the RHIMemoryStatisticsInterface
    void WritePoolsToJson(
        const AZStd::vector<MemoryStatistics::Pool>& pools, 
        rapidjson::Document& doc);

    //! Utility function to write captured pool data to a json document
    //! Available externally to the RHI through the RHIMemoryStatisticsInterface
    AZ::Outcome<void, AZStd::string> LoadPoolsFromJson(
        AZStd::vector<MemoryStatistics::Pool>& pools,
        AZStd::vector<AZ::RHI::MemoryStatistics::Heap>& heaps,
        rapidjson::Document& doc,
        const AZStd::string& fileName);

    //! Utility function to trigger an emergency dump of pool information to json.
    //! Intended to be used for gpu memory failure debugging.
    //! Available externally to the RHI through the RHIMemoryStatisticsInterface
    void DumpPoolInfoToJson();

    //! Utility function to get the DrawListTagRegistry
    RHI::DrawListTagRegistry* GetDrawListTagRegistry();
 
    //! Utility function to get the Name associated with a DrawListTag
    Name GetDrawListName(DrawListTag drawListTag);

    AZStd::string DrawListMaskToString(const RHI::DrawListMask& drawListMask);

}

