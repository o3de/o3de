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

#include <AtomLyIntegration/CommonFeatures/ScreenSpace/DeferredFogComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        void DeferredFogComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DeferredFogComponentConfig, ComponentConfig>()
                    ->Version(1)

                    // Auto-gen serialize context code...
#define SERIALIZE_CLASS DeferredFogComponentConfig
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ->Field(#Name, &SERIALIZE_CLASS::MemberName)                                                    \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef SERIALIZE_CLASS
                    ->Field("IsEnabled", &DeferredFogComponentConfig::m_enabled)
                    ->Field("UseNoiseTextureShaderOption", &DeferredFogComponentConfig::m_useNoiseTextureShaderOption)
                    ->Field("EnableFogLayerShaderOption", &DeferredFogComponentConfig::m_enableFogLayerShaderOption)
                    ;
            }
        }

        void DeferredFogComponentConfig::CopySettingsFrom(DeferredFogSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_SOURCE settings
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        MemberName = COPY_SOURCE->Get##Name();                                                          \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_SOURCE
        }

        void DeferredFogComponentConfig::CopySettingsTo(DeferredFogSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_TARGET settings
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        COPY_TARGET->Set##Name(MemberName);                                                             \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_TARGET
        }

    } // namespace Render
} // namespace AZ

