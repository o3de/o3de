/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AzCore/Math/Matrix4x4.h>

namespace AZ
{
    namespace Render
    {
        class HDRiSkyboxComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(HDRiSkyboxComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::HDRiSkyboxComponentConfig, "{AEAD8F5A-8D2F-47CD-B98C-C99541F7B229}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            Data::Asset<RPI::StreamingImageAsset> m_cubemapAsset;
            Data::AssetId m_cubemapAssetId;

            float m_exposure = 0.0f;
        };
    } // namespace Render
} // namespace AZ
