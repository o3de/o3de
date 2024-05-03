/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/Component.h>

#include <Atom/Feature/ParamMacros/ParamMacrosHowTo.inl>    // for education purposes
#include <Atom/Feature/ScreenSpace/DeferredFogSettingsInterface.h>



namespace AZ
{
    namespace Render
    {
        class EditorDeferredFogComponent;
        // The component config file containing the editor fog data for configuring full screen deferred fog.
        // The fog is calculated using the linear depth and turbulence texture with two blended
        // octaves that emulate the fog thickness and motion along the view ray direction.
        //
        // [GFX TODO] most methods in this class can be auto generated using a reflection mechanism
        //  based on the PerPass Srg.  Currently we don't have such a reflection system in place
        //  and so, a temporary approach demonstrating partial reflection through preprocessor
        //  macros is used.
        class DeferredFogComponentConfig final :
            public ComponentConfig 
        {
            friend class EditorDeferredFogComponent;

        public:
            AZ_CLASS_ALLOCATOR(DeferredFogComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::DeferredFogComponentConfig, "{3C2671FE-6027-4A1E-907B-F7E2B1B64F7B}", AZ::ComponentConfig );

            static void Reflect(ReflectContext* context);

            void CopySettingsFrom(DeferredFogSettingsInterface* settings);
            void CopySettingsTo(DeferredFogSettingsInterface* settings);

            // DeferredFogComponentConfigInterface overrides...
            void SetEnabled(bool value)
            {
                m_enabled = value;
            }
            bool GetIsEnabled()
            {
                return m_enabled;
            }

            void SetUseNoiseTextureShaderOption(bool value)
            {
                m_useNoiseTextureShaderOption = value;
            }
            bool GetUseNoiseTextureShaderOption()
            {
                return m_useNoiseTextureShaderOption;
            }

            void SetEnableFogLayerShaderOption(bool value)
            {
                m_enableFogLayerShaderOption = value;
            }
            bool GetEnableFogLayerShaderOption()
            {
                return m_enableFogLayerShaderOption;
            }

            bool SupportsFogDensity()
            {
                return m_fogMode == FogMode::Exponential || m_fogMode == FogMode::ExponentialSquared;
            }
            bool SupportsFogEnd()
            {
                return m_fogMode == FogMode::Linear;
            }

            // Generate Get / Set methods
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        virtual ValueType Get##Name() const { return MemberName; }                                      \
        virtual void Set##Name(ValueType val) { MemberName = val; }                                     \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:

            // SRG constants
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType MemberName = DefaultValue;                                                            \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            bool m_enabled = true;
            bool m_useNoiseTextureShaderOption = false;
            bool m_enableFogLayerShaderOption = false;
        };

    }
}
