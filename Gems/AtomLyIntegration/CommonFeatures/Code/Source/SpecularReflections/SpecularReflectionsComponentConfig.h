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
#include <Atom/Feature/SpecularReflections/SpecularReflectionsFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class SpecularReflectionsComponentSSRConfig final
        {
        public:
            AZ_RTTI(SpecularReflectionsComponentSSRConfig, "{B492A485-3FC2-4E33-8E5D-90885ACE9EDB}");
            AZ_CLASS_ALLOCATOR(SpecularReflectionsComponentSSRConfig, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            SSROptions m_options;
        };

        class SpecularReflectionsComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(SpecularReflectionsComponentConfig, "{02A8F0D0-1849-451D-B498-202B71373998}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(SpecularReflectionsComponentConfig, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            SpecularReflectionsComponentSSRConfig m_ssr;
        };
    }
}
