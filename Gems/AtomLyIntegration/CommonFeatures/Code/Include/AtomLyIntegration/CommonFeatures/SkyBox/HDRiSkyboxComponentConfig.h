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
#include <AzCore/Math/Matrix4x4.h>

namespace AZ
{
    namespace Render
    {
        class HDRiSkyboxComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::HDRiSkyboxComponentConfig, "{AEAD8F5A-8D2F-47CD-B98C-C99541F7B229}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            Data::Asset<RPI::StreamingImageAsset> m_cubemapAsset;
            float m_exposure = 0.0f;
        };
    } // namespace Render
} // namespace AZ
