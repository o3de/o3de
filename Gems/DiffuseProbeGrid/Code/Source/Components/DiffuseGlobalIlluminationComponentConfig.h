/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <DiffuseProbeGrid/DiffuseGlobalIlluminationFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseGlobalIlluminationComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(DiffuseGlobalIlluminationComponentConfig, "{0D0835D6-6094-4EF8-BEAC-5FF8A4E4C119}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(DiffuseGlobalIlluminationComponentConfig, SystemAllocator);

            static void Reflect(ReflectContext* context);

            DiffuseGlobalIlluminationQualityLevel m_qualityLevel = DiffuseGlobalIlluminationQualityLevel::Low;
        };
    }
}
