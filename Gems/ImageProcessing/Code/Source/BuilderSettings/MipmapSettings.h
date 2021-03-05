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

namespace ImageProcessing
{    
    struct MipmapSettings
    {
        AZ_TYPE_INFO(MipmapSettings, "{9239618E-23A6-43C8-9B87-50528CBFA6FF}");
        AZ_CLASS_ALLOCATOR(MipmapSettings, AZ::SystemAllocator, 0);
        bool operator!=(const MipmapSettings& other) const;
        bool operator==(const MipmapSettings& other) const;

        static void Reflect(AZ::ReflectContext* context);

        MipGenType m_type = MipGenType::blackmanHarris;

        //Unused or duplicated properties. We may want to move same properties from perset setting to here.
        AZ::Color m_borderColor;
        bool m_normalize;
        AZ::u32 m_streamableMips;
    };
} // namespace ImageProcessing