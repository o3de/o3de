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

namespace AZ
{
    namespace Render
    {
        //! Common settings for ImageBasedLightComponents and EditorImageBasedLightComponent.
        class ImageBasedLightComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(ImageBasedLightComponentConfig, "{2BD353A5-562B-4D84-9508-B2EFAFF1415E}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(ImageBasedLightComponentConfig, SystemAllocator);

            static void Reflect(ReflectContext* context);

            Data::Asset<RPI::StreamingImageAsset> m_specularImageAsset;
            Data::Asset<RPI::StreamingImageAsset> m_diffuseImageAsset;
            float m_exposure = 0.0f;
        };
    } // namespace Render
} // namespace AZ
