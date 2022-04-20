/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// RHIUtils is for dumping common functionality that is used in several places across the RHI
 
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Format.h>

#include <Atom/RHI/Device.h>

namespace AZ
{
    namespace RHI
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

        //! Returns if the current bakcend is null 
        bool IsNullRHI();

        //! Returns true if the Atom/GraphicsDevMode settings registry key is set
        bool IsGraphicsDevModeEnabled();
    }
}

