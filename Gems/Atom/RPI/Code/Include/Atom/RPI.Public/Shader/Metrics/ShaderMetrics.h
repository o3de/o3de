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

#include <AzCore/std/containers/vector.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>

#include "Atom/RPI.Reflect/Shader/ShaderVariantKey.h"

namespace AZ
{
    namespace RPI
    {
        class ShaderAsset;

        struct ShaderVariantRequest
        {
            AZ_TYPE_INFO(ShaderVariantRequest, "{E85512A6-1B6C-4082-857F-6D9FA6247FD6}");
            static void Reflect(AZ::ReflectContext* context);

            //! The ID of the shader.
            AZ::Data::AssetId m_shaderId;

            //! The name of the shader.
            Name m_shaderName;

            //! The ID of the shader variant.
            ShaderVariantId m_shaderVariantId;

            //! The index of the shader variant.
            ShaderVariantStableId m_shaderVariantStableId;

            //! The number of dynamic options in the variant.
            uint32_t m_dynamicOptionCount;

            //! The number of requests for the variant.
            uint32_t m_requestCount;
        };

        struct ShaderVariantMetrics
        {
            AZ_TYPE_INFO(ShaderVariantMetrics, "{F368F633-449D-4B31-83E8-A44C945C9B78}");
            AZ_CLASS_ALLOCATOR(ShaderVariantMetrics, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            AZStd::vector<ShaderVariantRequest> m_requests;
        };

        //////////////////////////////////////////////////////////////////////////
    } // namespace RPI

} // namespace AZ
