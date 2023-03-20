/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentConfig.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void ImageBasedLightComponentConfig::Reflect(ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ImageBasedLightComponentConfig, ComponentConfig>()
                    ->Version(1)
                    ->Field("diffuseImageAsset", &ImageBasedLightComponentConfig::m_diffuseImageAsset)
                    ->Field("specularImageAsset", &ImageBasedLightComponentConfig::m_specularImageAsset)
                    ->Field("exposure", &ImageBasedLightComponentConfig::m_exposure)
                    ;
            }
        }

    } // namespace Render
} // namespace AZ
