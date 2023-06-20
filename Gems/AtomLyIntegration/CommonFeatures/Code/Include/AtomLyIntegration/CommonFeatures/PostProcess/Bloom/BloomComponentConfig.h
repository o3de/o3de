/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/Bloom/BloomSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        class BloomComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(BloomComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::BloomComponentConfig, "{23545754-0FAE-4220-99AF-0AA0045F4D8D}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/Bloom/BloomParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/Bloom/BloomParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(BloomSettingsInterface* settings);
            void CopySettingsTo(BloomSettingsInterface* settings);

            bool ArePropertiesReadOnly() const { return !m_enabled; }
        };
    }
}
