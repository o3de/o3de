/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/ImageProcessing/ImageProcessingDefines.h>
#include <Atom/ImageProcessing/PixelFormats.h>

namespace ImageProcessingAtom
{
    //! default settings for platform
    struct PlatformSetting
    {
        AZ_TYPE_INFO(PlatformSetting, "{95FBE763-C5CD-4C40-964F-9D34E3AB2138}");
        AZ_CLASS_ALLOCATOR(PlatformSetting, AZ::SystemAllocator);

        //! Platform's name
        PlatformName m_name;

        //! pixel formats supported for the platform
        AZStd::list<EPixelFormat> m_availableFormat;
    };
} // namespace ImageProcessingAtom
