/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <ACES/Aces.h>
#include <Atom/Feature/PostProcess/LookModification/LookModificationSettingsInterface.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ
{
    namespace Render
    {
        class LookModificationComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(LookModificationComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::LookModificationComponentConfig, "{604D14B8-6374-4FE0-8F31-03A37B238429}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(LookModificationSettingsInterface* settings);
            void CopySettingsTo(LookModificationSettingsInterface* settings);

            bool ArePropertiesReadOnly() const { return !m_enabled; }

            bool IsUsingCustomShaper() const {
                return m_shaperPresetType == ShaperPresetType::LinearCustomRange
                    || m_shaperPresetType == ShaperPresetType::Log2CustomRange;
            }
        };
    }
}
