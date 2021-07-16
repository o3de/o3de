/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>

namespace AZ
{
    namespace RPI
    {
        //! Custom data for the DownsampleMipChainPass. Should be specified in the PassRequest.
        struct DownsampleMipChainPassData
            : public PassData
        {
            AZ_RTTI(DownsampleMipChainPassData, "{EB240B6F-91CB-401A-A099-8F06329BDF35}", PassData);
            AZ_CLASS_ALLOCATOR(DownsampleMipChainPassData, SystemAllocator, 0);

            DownsampleMipChainPassData() = default;
            virtual ~DownsampleMipChainPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<DownsampleMipChainPassData, PassData>()
                        ->Version(0)
                        ->Field("ShaderAsset", &DownsampleMipChainPassData::m_shaderReference)
                        ;
                }
            }

            //! Reference to the Compute Shader that will be used by DownsampleMipeChainPass
            AssetReference m_shaderReference;
        };
    } // namespace RPI
} // namespace AZ

