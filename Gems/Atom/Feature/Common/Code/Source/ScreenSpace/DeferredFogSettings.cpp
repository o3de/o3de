/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <ScreenSpace/DeferredFogSettings.h>
#include <ScreenSpace/DeferredFogPass.h>

namespace AZ
{
    namespace Render
    {
        DeferredFogSettings::DeferredFogSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor) {}

        DeferredFogSettings::DeferredFogSettings()
            : PostProcessBase(nullptr) {}

        AZ::Data::Instance<AZ::RPI::StreamingImage> DeferredFogSettings::LoadStreamingImage(
            const char* textureFilePath, [[maybe_unused]] const char* sampleName)
        {
            return AZ::RPI::LoadStreamingTexture(textureFilePath);
        }

        void DeferredFogSettings::OnSettingsChanged()
        {
            m_needUpdate = true;   // even if disabled, mark it for when it'll become enabled
        }

        void DeferredFogSettings::SetEnabled(bool value) 
        {
            m_enabled = value;
            OnSettingsChanged();
        }

        //-------------------------------------------
        // Getters / setters macro
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                              \
        ValueType DeferredFogSettings::Get##Name() const                                            \
        {                                                                                           \
            return MemberName;                                                                      \
        }                                                                                           \
        void DeferredFogSettings::Set##Name(ValueType val)                                          \
        {                                                                                           \
            MemberName = val;                                                                       \
            OnSettingsChanged();                                                                    \
        }                                                                                           \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        //-------------------------------------------

        void DeferredFogSettings::ApplySettingsTo(DeferredFogSettings* target, [[maybe_unused]] float alpha) const
        {
            AZ_Assert(target != nullptr, "DeferredFogSettings::ApplySettingsTo called with nullptr as argument.");
            if (!target)
            {
                return;
            }

            // For now the fog should be singleton - later on we'd need a better blending functionality 
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                              \
            target->Set##Name(MemberName);                                                          \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        }
    } // namespace Render
} // namespace AZ

