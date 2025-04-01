/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        class DepthOfFieldComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(DepthOfFieldComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::DepthOfFieldComponentConfig, "{41E878A3-7DE6-4F27-AD14-FC115DE506F5}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(DepthOfFieldSettingsInterface* settings);
            void CopySettingsTo(DepthOfFieldSettingsInterface* settings);

            bool IsCameraEntityInvalid() const { return !m_cameraEntityId.IsValid(); }
            bool ArePropertiesReadOnly() const { return !m_enabled || IsCameraEntityInvalid(); }
            bool IsAutoFocusReadOnly() const { return !m_enableAutoFocus || m_focusedEntityId.IsValid() || ArePropertiesReadOnly(); }
            bool IsFocusedEntityReadonly() const { return !m_enableAutoFocus || ArePropertiesReadOnly(); }
            bool IsFocusDistanceReadOnly() const { return m_enableAutoFocus || ArePropertiesReadOnly(); }
        };
    }
}
