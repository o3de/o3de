/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/SkyBox/HDRiSkyboxComponentConfig.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Matrix4x4.h>

namespace AZ
{
    namespace Render
    {
        void HDRiSkyboxComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<HDRiSkyboxComponentConfig, ComponentConfig>()
                    ->Version(7)
                    ->Field("CubemapAsset", &HDRiSkyboxComponentConfig::m_cubemapAsset)
                    ->Field("Exposure", &HDRiSkyboxComponentConfig::m_exposure)
                    ;
            }
        }
    }
}
