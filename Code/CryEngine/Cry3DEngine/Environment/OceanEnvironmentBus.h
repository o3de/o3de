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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>

#include <I3DEngine.h>
#include <ISystem.h>
#include <OceanConstants.h>

namespace AZ
{
    /**
    * Feature toggle for the ocean feature(s)
    */
    class OceanFeatureToggle
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        virtual ~OceanFeatureToggle() = default;

        virtual bool OceanComponentEnabled() const { return false; }

    };
    using OceanFeatureToggleBus = AZ::EBus<OceanFeatureToggle>;

    /*!
    * Messages services for environment data points
    * Note: The Gem for Water is meant to override this when enabled in a project
    */
    class OceanEnvironmentRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using MutexType = AZStd::recursive_mutex;

        //////////////////////////////////////////////////////////////////////////

        // flags for toggling ocean reflections
        enum class ReflectionFlags
        {
            Entities = SRenderingPassInfo::ENTITIES,
            StaticObjects = SRenderingPassInfo::STATIC_OBJECTS
        };

        // Ocean requests

        virtual bool OceanIsEnabled() const = 0;

        // Fast option - use if just ocean height required
        virtual float GetOceanLevel() const = 0;
        virtual void SetOceanLevel(float oceanLevel) = 0;
        virtual float GetOceanLevelOrDefault(const float defaultValue) const = 0;
        
        // This will return ocean height or water volume height, optional for accurate water height query
        virtual float GetWaterLevel(const Vec3& position) const = 0;

        // Only use for Accurate query - this will return exact ocean height
        virtual float GetAccurateOceanHeight(const Vec3& position) const = 0;

        // gets the amount of water tessellation
        virtual int GetWaterTessellationAmount() const = 0;
        virtual void SetWaterTessellationAmount(int amount) = 0;

        // the ocean material asset
        virtual const AZStd::string& GetOceanMaterialName() const = 0;
        virtual void SetOceanMaterialName(const AZStd::string& matName) = 0;

        // Animation data
        virtual float GetAnimationWindDirection() const = 0;
        virtual float GetAnimationWindSpeed() const = 0;
        virtual float GetAnimationWavesSpeed() const = 0;
        virtual float GetAnimationWavesSize() const = 0;
        virtual float GetAnimationWavesAmount() const = 0;
        virtual void SetAnimationWindDirection(float dir) = 0;
        virtual void SetAnimationWindSpeed(float speed) = 0;
        virtual void SetAnimationWavesSpeed(float speed) = 0;
        virtual void SetAnimationWavesSize(float size) = 0;
        virtual void SetAnimationWavesAmount(float amount) = 0;
        
        // Ocean reflection
        virtual void ApplyReflectRenderFlags(int& flags) const = 0;
        virtual bool GetReflectRenderFlag(ReflectionFlags flag) const = 0;
        virtual float GetReflectResolutionScale() const = 0;
        virtual bool GetReflectionAnisotropic() const = 0;
        virtual void SetReflectRenderFlag(ReflectionFlags flag, bool value) = 0;
        virtual void SetReflectResolutionScale(float scale) = 0;
        virtual void SetReflectionAnisotropic(bool enabled) = 0;
        
        // Ocean bottom
        virtual bool GetUseOceanBottom() const = 0;
        virtual void SetUseOceanBottom(bool use) = 0;

        // Underwater Effects
        virtual bool GetGodRaysEnabled() const = 0;
        virtual void SetGodRaysEnabled(bool enabled) = 0;
        virtual float GetUnderwaterDistortion() const = 0;
        virtual void SetUnderwaterDistortion(float) = 0;

        // Caustics
        virtual bool  GetCausticsEnabled() const = 0;
        virtual float GetCausticsDepth() const = 0;
        virtual float GetCausticsIntensity() const = 0;
        virtual float GetCausticsTiling() const = 0;
        virtual float GetCausticsDistanceAttenuation() const = 0;
        virtual void SetCausticsEnabled(bool enable)  = 0;
        virtual void SetCausticsDepth(float depth)  = 0;
        virtual void SetCausticsIntensity(float intensity)  = 0;
        virtual void SetCausticsTiling(float tiling)  = 0;
        virtual void SetCausticsDistanceAttenuation(float dist) = 0;

        // Ocean fog data
        virtual AZ::Color GetFogColorPremultiplied() const = 0;
        virtual AZ::Color GetFogColor() const = 0;
        virtual void SetFogColor(const AZ::Color& fogColor) = 0;
        virtual float GetFogColorMultiplier() const = 0;
        virtual void SetFogColorMultiplier(float fogMultiplier) = 0;
        virtual AZ::Color GetNearFogColor() const = 0;
        virtual void SetNearFogColor(const AZ::Color& nearColor) = 0;
        virtual float GetFogDensity() const = 0;
        virtual void SetFogDensity(float density) = 0;
    };

    using OceanEnvironmentBus = AZ::EBus<OceanEnvironmentRequests>;

} // namespace AZ

namespace OceanToggle
{
    /**
    * As long as the Water gem is in a preview state, the legacy code and data will be protected by this feature toggle check.
    */
    AZ_INLINE bool IsActive()
    {
        bool bHasOceanFeature = false;
        AZ::OceanFeatureToggleBus::BroadcastResult(bHasOceanFeature, &AZ::OceanFeatureToggleBus::Events::OceanComponentEnabled);
        return bHasOceanFeature;
    }
} // namespace OceanToggle

namespace OceanRequest
{

    AZ_INLINE bool OceanIsEnabled()
    {
        bool enabled = false;
        AZ::OceanEnvironmentBus::BroadcastResult(enabled, &AZ::OceanEnvironmentBus::Events::OceanIsEnabled);
        return enabled;
    }

    // Ocean level

    AZ_INLINE float GetOceanLevel()
    {
        float fWaterLevel = AZ::OceanConstants::s_HeightUnknown;
        AZ::OceanEnvironmentBus::BroadcastResult(fWaterLevel, &AZ::OceanEnvironmentBus::Events::GetOceanLevel);
        return fWaterLevel;
    }

    AZ_INLINE float GetOceanLevelOrDefault(const float defaultValue)
    {
        return OceanIsEnabled() ? GetOceanLevel() : defaultValue;
    }

    AZ_INLINE float GetWaterLevel(const Vec3& position)
    {
        float fWaterLevel = AZ::OceanConstants::s_HeightUnknown;
        AZ::OceanEnvironmentBus::BroadcastResult(fWaterLevel, &AZ::OceanEnvironmentBus::Events::GetWaterLevel, position);
        return fWaterLevel;
    }

    AZ_INLINE float GetAccurateOceanHeight(const Vec3& position)
    {
        float fWaterLevel = AZ::OceanConstants::s_HeightUnknown;
        AZ::OceanEnvironmentBus::BroadcastResult(fWaterLevel, &AZ::OceanEnvironmentBus::Events::GetAccurateOceanHeight, position);
        return fWaterLevel;
    }

    // the ocean material

    AZ_INLINE AZStd::string GetOceanMaterialName()
    {
        AZStd::string value = "EngineAssets/Materials/Water/Ocean_default.mtl";
        AZ::OceanEnvironmentBus::BroadcastResult(value, &AZ::OceanEnvironmentBus::Events::GetOceanMaterialName);
        return value;
    }

    // Wave animation data

    AZ_INLINE float GetWavesAmount()
    {
        float value = AZ::OceanConstants::s_animationWavesAmountDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(value, &AZ::OceanEnvironmentBus::Events::GetAnimationWavesAmount);
        return value;
    }

    AZ_INLINE float GetWavesSpeed()
    {
        float value = AZ::OceanConstants::s_animationWavesSpeedDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(value, &AZ::OceanEnvironmentBus::Events::GetAnimationWavesSpeed);
        return value;
    }

    AZ_INLINE float GetWavesSize()
    {
        float value = AZ::OceanConstants::s_animationWavesSizeDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(value, &AZ::OceanEnvironmentBus::Events::GetAnimationWavesSize);
        return value;
    }

    AZ_INLINE float GetWindDirection()
    {
        float value = AZ::OceanConstants::s_animationWindDirectionDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(value, &AZ::OceanEnvironmentBus::Events::GetAnimationWindDirection);
        return value;
    }

    AZ_INLINE float GetWindSpeed()
    {
        float value = AZ::OceanConstants::s_animationWindSpeedDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(value, &AZ::OceanEnvironmentBus::Events::GetAnimationWindSpeed);
        return value;
    }

    // Ocean bottom

    AZ_INLINE bool GetUseOceanBottom()
    {
        bool useOceanBottom = AZ::OceanConstants::s_UseOceanBottom;
        AZ::OceanEnvironmentBus::BroadcastResult(useOceanBottom, &AZ::OceanEnvironmentBus::Events::GetUseOceanBottom);
        return useOceanBottom;
    }


    AZ_INLINE bool GetGodRaysEnabled()
    {
        bool godRaysEnabled = AZ::OceanConstants::s_GodRaysEnabled;
        AZ::OceanEnvironmentBus::BroadcastResult(godRaysEnabled, &AZ::OceanEnvironmentBus::Events::GetGodRaysEnabled);
        return godRaysEnabled;
    }

    AZ_INLINE float GetUnderwaterDistortion()
    {
        float underwaterDistortion = AZ::OceanConstants::s_UnderwaterDistortion;
        AZ::OceanEnvironmentBus::BroadcastResult(underwaterDistortion, &AZ::OceanEnvironmentBus::Events::GetUnderwaterDistortion);
        return underwaterDistortion;
    }
    
    // Cuastics

    AZ_INLINE bool GetCausticsEnabled()
    {
        bool causticsEnabled = false;
        AZ::OceanEnvironmentBus::BroadcastResult(causticsEnabled, &AZ::OceanEnvironmentBus::Events::GetCausticsEnabled);
        return causticsEnabled;
    }

    AZ_INLINE float GetCausticsDepth()
    {
        float causticsDepth = AZ::OceanConstants::s_CausticsDepthDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(causticsDepth, &AZ::OceanEnvironmentBus::Events::GetCausticsDepth);
        return causticsDepth;
    }

    AZ_INLINE float GetCausticsIntensity()
    {
        float causticsIntensity = AZ::OceanConstants::s_CausticsIntensityDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(causticsIntensity, &AZ::OceanEnvironmentBus::Events::GetCausticsIntensity);
        return causticsIntensity;
    }

    AZ_INLINE float GetCausticsTiling()
    {
        float causticsTiling = AZ::OceanConstants::s_CausticsTilingDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(causticsTiling, &AZ::OceanEnvironmentBus::Events::GetCausticsTiling);
        return causticsTiling;
    }

    AZ_INLINE float GetCausticsDistanceAttenuation()
    {
        float causticsDistanceAtten = AZ::OceanConstants::s_CausticsDistanceAttenDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(causticsDistanceAtten, &AZ::OceanEnvironmentBus::Events::GetCausticsDistanceAttenuation);
        return causticsDistanceAtten;
    }

    // Ocean fog
    AZ_INLINE AZ::Vector3 GetFogColorPremultiplied()
    {
        AZ::Color fogColor = AZ::OceanConstants::s_oceanFogColorDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(fogColor, &AZ::OceanEnvironmentBus::Events::GetFogColorPremultiplied);
        return fogColor.GetAsVector3();
    }

    AZ_INLINE AZ::Vector3 GetNearFogColor()
    {
        AZ::Color nearFogColor = AZ::OceanConstants::s_oceanNearFogColorDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(nearFogColor, &AZ::OceanEnvironmentBus::Events::GetNearFogColor);
        return nearFogColor.GetAsVector3();
    }

    AZ_INLINE float GetFogDensity()
    {
        float fogDensity = AZ::OceanConstants::s_oceanFogDensityDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(fogDensity, &AZ::OceanEnvironmentBus::Events::GetFogDensity);
        return fogDensity;
    }

} // namespace OceanRequest
