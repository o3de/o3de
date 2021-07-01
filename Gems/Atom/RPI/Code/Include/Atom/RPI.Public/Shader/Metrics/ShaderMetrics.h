/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
