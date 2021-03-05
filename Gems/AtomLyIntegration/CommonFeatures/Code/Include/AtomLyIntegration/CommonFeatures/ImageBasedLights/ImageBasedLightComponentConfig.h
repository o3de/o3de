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
            AZ_CLASS_ALLOCATOR(ImageBasedLightComponentConfig, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            Data::Asset<RPI::StreamingImageAsset> m_specularImageAsset;
            Data::Asset<RPI::StreamingImageAsset> m_diffuseImageAsset;
            float m_exposure = 0.0f;
        };
    } // namespace Render
} // namespace AZ
