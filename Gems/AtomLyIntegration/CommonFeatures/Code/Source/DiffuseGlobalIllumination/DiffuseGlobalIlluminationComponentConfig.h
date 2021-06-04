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
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <Atom/Feature/DiffuseGlobalIllumination/DiffuseGlobalIlluminationFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseGlobalIlluminationComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(DiffuseGlobalIlluminationComponentConfig, "{0D0835D6-6094-4EF8-BEAC-5FF8A4E4C119}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(DiffuseGlobalIlluminationComponentConfig, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            DiffuseGlobalIlluminationQualityLevel m_qualityLevel;
        };
    }
}
