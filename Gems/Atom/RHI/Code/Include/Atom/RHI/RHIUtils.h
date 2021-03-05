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
#pragma once

// RHIUtils is for dumping common functionality that is used in several places across the RHI
 
#include <AtomCore/Instance/Instance.h>

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
    }
}

