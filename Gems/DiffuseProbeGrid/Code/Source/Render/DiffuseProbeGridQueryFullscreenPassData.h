/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

namespace AZ
{
    namespace Render
    {
        //! Custom data for the DiffuseProbeGridQueryFullscreenPass, specified in the PassRequest.
        struct DiffuseProbeGridQueryFullscreenPassData
            : public RPI::RenderPassData
        {
            AZ_RTTI(DiffuseProbeGridQueryFullscreenPassData, "{170F11E4-F76C-41E2-90A8-71568A189B98}", RPI::RenderPassData);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridQueryFullscreenPassData, SystemAllocator, 0);

            DiffuseProbeGridQueryFullscreenPassData() = default;
            virtual ~DiffuseProbeGridQueryFullscreenPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<DiffuseProbeGridQueryFullscreenPassData, RenderPassData>()
                        ->Version(1)
                        ->Field("Use Albedo Texture", &DiffuseProbeGridQueryFullscreenPassData::m_useAlbedoTexture)
                        ;
                }
            }

            bool m_useAlbedoTexture = false;
        };
    } // namespace RPI
} // namespace AZ

