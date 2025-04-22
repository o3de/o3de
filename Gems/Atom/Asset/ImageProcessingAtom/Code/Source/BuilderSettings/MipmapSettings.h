/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/ImageProcessing/ImageProcessingDefines.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Math/Color.h>

namespace ImageProcessingAtom
{
    struct MipmapSettings
    {
        AZ_TYPE_INFO(MipmapSettings, "{37C05CB4-365B-4F70-9620-B9017DB0A8C2}");
        AZ_CLASS_ALLOCATOR(MipmapSettings, AZ::SystemAllocator);
        bool operator!=(const MipmapSettings& other) const;
        bool operator==(const MipmapSettings& other) const;

        static void Reflect(AZ::ReflectContext* context);

        MipGenType m_type = MipGenType::blackmanHarris;
    };
} // namespace ImageProcessingAtom
