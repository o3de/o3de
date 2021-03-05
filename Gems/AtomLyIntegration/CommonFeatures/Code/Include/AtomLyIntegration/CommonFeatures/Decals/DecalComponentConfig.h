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
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>

namespace AZ
{
    namespace Render
    {
        struct DecalComponentConfig final
            : public ComponentConfig
        {
            AZ_RTTI(DecalComponentConfig, "{5DFDC832-38B6-4F46-8C53-4CA0C82BC0AB}", ComponentConfig);
            static void Reflect(AZ::ReflectContext* context);

            static const uint8_t DefaultDecalSortKey = 16;

            Data::Asset<RPI::MaterialAsset> m_materialAsset;
            float m_attenuationAngle = 1.0f;
            float m_opacity = 1.0f;
            // Decals with a larger sort key appear over top of smaller sort keys            
            uint8_t m_sortKey = DefaultDecalSortKey;
        };
    }
}
