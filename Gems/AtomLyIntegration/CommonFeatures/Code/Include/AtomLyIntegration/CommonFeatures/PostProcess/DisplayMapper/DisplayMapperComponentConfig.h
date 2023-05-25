/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>

namespace AZ
{
    namespace Render
    {
        class DisplayMapperComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(DisplayMapperComponentConfig, "{98560BEC-EA7F-4600-AA55-F76FDC07E950}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(DisplayMapperComponentConfig, SystemAllocator);

            static void Reflect(ReflectContext* context);

            DisplayMapperOperationType m_displayMapperOperation = DisplayMapperOperationType::Aces;
            bool m_ldrColorGradingLutEnabled = false;
            Data::Asset<RPI::AnyAsset> m_ldrColorGradingLut = {};
            AcesParameterOverrides m_acesParameterOverrides;
        };
    }
}
