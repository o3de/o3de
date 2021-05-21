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
            AZ_CLASS_ALLOCATOR(DisplayMapperComponentConfig, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            DisplayMapperOperationType m_displayMapperOperation = DisplayMapperOperationType::Aces;
            bool m_ldrColorGradingLutEnabled = false;
            Data::Asset<RPI::AnyAsset> m_ldrColorGradingLut = {};
            AcesParameterOverrides m_acesParameterOverrides;
        };
    }
}
