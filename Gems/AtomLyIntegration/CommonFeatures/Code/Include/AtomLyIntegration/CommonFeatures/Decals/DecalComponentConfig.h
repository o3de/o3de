/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR(DecalComponentConfig, SystemAllocator)
            AZ_RTTI(DecalComponentConfig, "{5DFDC832-38B6-4F46-8C53-4CA0C82BC0AB}", ComponentConfig);
            static void Reflect(AZ::ReflectContext* context);

            static const uint8_t DefaultDecalSortKey = 16;

            Data::Asset<RPI::MaterialAsset> m_materialAsset;
            float m_attenuationAngle = 1.0f;
            float m_opacity = 1.0f;
            float m_normalMapOpacity = 1.0f;
            // Decals with a larger sort key appear over top of smaller sort keys            
            uint8_t m_sortKey = DefaultDecalSortKey;

            AZ::Vector3 m_decalColor = AZ::Vector3::CreateOne();
            float m_decalColorFactor = 1.0f;
        };
    }
}
