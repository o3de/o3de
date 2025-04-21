/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

namespace AZ
{
    namespace RPI
    {
        //! Custom data for the FullscreenTrianglePass. Should be specified in the PassRequest.
        struct ATOM_RPI_REFLECT_API FullscreenTrianglePassData
            : public RenderPassData
        {
            AZ_RTTI(FullscreenTrianglePassData, "{564738A1-9690-4446-8CC1-615EA2567434}", RenderPassData);
            AZ_CLASS_ALLOCATOR(FullscreenTrianglePassData, SystemAllocator);

            FullscreenTrianglePassData() = default;
            virtual ~FullscreenTrianglePassData() = default;

            static void Reflect(ReflectContext* context);

            //! Reference to the shader asset used by the fullscreen triangle pass
            AssetReference m_shaderAsset;

            //! Stencil reference value to use for the draw call
            uint32_t m_stencilRef = 0;
        };
    } // namespace RPI
} // namespace AZ

