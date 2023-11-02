/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>

#include <Atom/Feature/PostProcess/LookModification/LookModificationSettingsInterface.h>

#include <PostProcess/PostProcessBase.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <ACES/Aces.h>

namespace AZ
{
    namespace Render
    {
        class PostProcessSettings;

        struct LutBlendItem
        {
            //! The intensity of the LUT considered by itself compared to the ungraded base color.
            float m_intensity = 0.0f;
            //! The override intensity of this LUT over lower priority LUTs (based on the post process layers).
            //! During LUT blending, this override intensity is considered in conjunction with the LUTs own intensity.
            float m_overrideStrength = 0.0;
            //! Asset ID of LUT
            Data::Asset<RPI::AnyAsset> m_asset;
            //! Shaper preset type
            ShaperPresetType m_shaperPreset = AZ::Render::ShaperPresetType::Log2_48Nits;
            //! When shaper preset is custom, these values set min and max exposure.
            float m_customMinExposure = -6.5;
            float m_customMaxExposure = 6.5;
            HashValue64 GetHash(HashValue64 seed) const;
        };

        //! The post process sub-settings class for the look modification feature
        class LookModificationSettings final
            : public LookModificationSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(LookModificationSettings, "{244F5635-C5CA-412F-AD3D-49D55A771EB1}", LookModificationSettingsInterface, PostProcessBase);
            AZ_CLASS_ALLOCATOR(LookModificationSettings, SystemAllocator);

            static const uint32_t MaxBlendLuts = 4;

            LookModificationSettings(PostProcessFeatureProcessor* featureProcessor);
            ~LookModificationSettings() = default;

            //! LookModificationSettingsInterface overrides...
            void OnConfigChanged() override;

            //! Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(LookModificationSettings* target, float alpha) const;

            //! Add a LUT blending item to the stack
            void AddLutBlend(LutBlendItem lutBlendItem);

            //! Should be called to prepare the contents in the LUT blending stack before blending.
            void PrepareLutBlending();

            //! Returns the size of the LUT blending stack
            size_t GetLutBlendStackSize();

            //! Retrive the LUT blending item from the stack at the given index
            LutBlendItem& GetLutBlendItem(size_t lutIndex);

            //! Get a hash for this setting
            HashValue64 GetHash() const;

            // Generate all getters and override setters.
            // Declare non-override setters, which will be defined in the .cpp
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType Get##Name() const override { return MemberName; }                                     \
        void Set##Name(ValueType val) override                                                          \
        {                                                                                               \
            MemberName = val;                                                                           \
        }                                                                                               \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        OverrideValueType Get##Name##Override() const override { return MemberName##Override; }         \
        void Set##Name##Override(OverrideValueType val) override { MemberName##Override = val; }        \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;

            AZStd::vector<LutBlendItem> m_lutBlendStack;
            bool m_preparedForBlending = false;
        };

    }
}
