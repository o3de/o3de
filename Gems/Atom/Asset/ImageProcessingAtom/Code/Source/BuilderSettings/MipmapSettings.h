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

#include <BuilderSettings/ImageProcessingDefines.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Math/Color.h>

namespace ImageProcessingAtom
{
    struct MipmapSettings
    {
        AZ_TYPE_INFO(MipmapSettings, "{37C05CB4-365B-4F70-9620-B9017DB0A8C2}");
        AZ_CLASS_ALLOCATOR(MipmapSettings, AZ::SystemAllocator, 0);
        bool operator!=(const MipmapSettings& other) const;
        bool operator==(const MipmapSettings& other) const;

        static void Reflect(AZ::ReflectContext* context);

        MipGenType m_type = MipGenType::blackmanHarris;
    };
} // namespace ImageProcessingAtom
