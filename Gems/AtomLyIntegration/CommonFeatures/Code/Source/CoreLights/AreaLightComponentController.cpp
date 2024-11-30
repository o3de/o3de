/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/AreaLightComponentController.h>
#include <CoreLights/CapsuleLightDelegate.h>
#include <CoreLights/DiskLightDelegate.h>
#include <CoreLights/PolygonLightDelegate.h>
#include <CoreLights/QuadLightDelegate.h>
#include <CoreLights/SimplePointLightDelegate.h>
#include <CoreLights/SimpleSpotLightDelegate.h>
#include <CoreLights/SphereLightDelegate.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AtomLyIntegration/CommonFeatures/CoreLights/CoreLightsConstants.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>

namespace AZ::Render
{
    void AreaLightComponentController::Reflect(ReflectContext* context)
    {
        AreaLightComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<AreaLightComponentController>()
                ->Version(1)
                ->Field("Configuration", &AreaLightComponentController::m_configuration);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AreaLightRequestBus>("AreaLightRequestBus")
                ->Event("GetAttenuationRadius", &AreaLightRequestBus::Events::GetAttenuationRadius)
                ->Event("SetAttenuationRadius", &AreaLightRequestBus::Events::SetAttenuationRadius)
                ->Event("SetAttenuationRadiusMode", &AreaLightRequestBus::Events::SetAttenuationRadiusMode)
                ->Event("GetColor", &AreaLightRequestBus::Events::GetColor)
                ->Event("SetColor", &AreaLightRequestBus::Events::SetColor)
                ->Event("GetEmitsLightBothDirections", &AreaLightRequestBus::Events::GetLightEmitsBothDirections)
                ->Event("SetEmitsLightBothDirections", &AreaLightRequestBus::Events::SetLightEmitsBothDirections)
                ->Event("GetUseFastApproximation", &AreaLightRequestBus::Events::GetUseFastApproximation)
                ->Event("SetUseFastApproximation", &AreaLightRequestBus::Events::SetUseFastApproximation)
                ->Event("GetIntensity", &AreaLightRequestBus::Events::GetIntensity)
                ->Event("SetIntensity", static_cast<void(AreaLightRequestBus::Events::*)(float)>(&AreaLightRequestBus::Events::SetIntensity))
                ->Event("SetIntensityAndMode", &AreaLightRequestBus::Events::SetIntensityAndMode)
                ->Event("GetIntensityMode", &AreaLightRequestBus::Events::GetIntensityMode)
                ->Event("ConvertToIntensityMode", &AreaLightRequestBus::Events::ConvertToIntensityMode)

                ->Event("GetEnableShutters", &AreaLightRequestBus::Events::GetEnableShutters)
                ->Event("SetEnableShutters", &AreaLightRequestBus::Events::SetEnableShutters)
                ->Event("GetInnerShutterAngle", &AreaLightRequestBus::Events::GetInnerShutterAngle)
                ->Event("SetInnerShutterAngle", &AreaLightRequestBus::Events::SetInnerShutterAngle)
                ->Event("GetOuterShutterAngle", &AreaLightRequestBus::Events::GetOuterShutterAngle)
                ->Event("SetOuterShutterAngle", &AreaLightRequestBus::Events::SetOuterShutterAngle)

                ->Event("GetEnableShadow", &AreaLightRequestBus::Events::GetEnableShadow)
                ->Event("SetEnableShadow", &AreaLightRequestBus::Events::SetEnableShadow)
                ->Event("GetShadowBias", &AreaLightRequestBus::Events::GetShadowBias)
                ->Event("SetShadowBias", &AreaLightRequestBus::Events::SetShadowBias)
                ->Event("GetNormalShadowBias", &AreaLightRequestBus::Events::GetNormalShadowBias)
                ->Event("SetNormalShadowBias", &AreaLightRequestBus::Events::SetNormalShadowBias)
                ->Event("GetShadowmapMaxSize", &AreaLightRequestBus::Events::GetShadowmapMaxSize)
                ->Event("SetShadowmapMaxSize", &AreaLightRequestBus::Events::SetShadowmapMaxSize)
                ->Event("GetShadowFilterMethod", &AreaLightRequestBus::Events::GetShadowFilterMethod)
                ->Event("SetShadowFilterMethod", &AreaLightRequestBus::Events::SetShadowFilterMethod)
                ->Event("GetFilteringSampleCount", &AreaLightRequestBus::Events::GetFilteringSampleCount)
                ->Event("SetFilteringSampleCount", &AreaLightRequestBus::Events::SetFilteringSampleCount)
                ->Event("GetEsmExponent", &AreaLightRequestBus::Events::GetEsmExponent)
                ->Event("SetEsmExponent", &AreaLightRequestBus::Events::SetEsmExponent)
                ->Event("GetShadowCachingMode", &AreaLightRequestBus::Events::GetShadowCachingMode)
                ->Event("SetShadowCachingMode", &AreaLightRequestBus::Events::SetShadowCachingMode)

                ->Event("GetAffectsGI", &AreaLightRequestBus::Events::GetAffectsGI)
                ->Event("SetAffectsGI", &AreaLightRequestBus::Events::SetAffectsGI)
                ->Event("GetAffectsGIFactor", &AreaLightRequestBus::Events::GetAffectsGIFactor)
                ->Event("SetAffectsGIFactor", &AreaLightRequestBus::Events::SetAffectsGIFactor)

                ->Event("GetLightingChannelMask", &AreaLightRequestBus::Events::GetLightingChannelMask)
                ->Event("SetLightingChannelMask", &AreaLightRequestBus::Events::SetLightingChannelMask)
                ->VirtualProperty("AttenuationRadius", "GetAttenuationRadius", "SetAttenuationRadius")
                ->VirtualProperty("Color", "GetColor", "SetColor")
                ->VirtualProperty("EmitsLightBothDirections", "GetEmitsLightBothDirections", "SetEmitsLightBothDirections")
                ->VirtualProperty("UseFastApproximation", "GetUseFastApproximation", "SetUseFastApproximation")
                ->VirtualProperty("Intensity", "GetIntensity", "SetIntensity")
                
                ->VirtualProperty("ShuttersEnabled", "GetEnableShutters", "SetEnableShutters")
                ->VirtualProperty("InnerShutterAngle", "GetInnerShutterAngle", "SetInnerShutterAngle")
                ->VirtualProperty("OuterShutterAngle", "GetOuterShutterAngle", "SetOuterShutterAngle")

                ->VirtualProperty("ShadowsEnabled", "GetEnableShadow", "SetEnableShadow")
                ->VirtualProperty("ShadowBias", "GetShadowBias", "SetShadowBias")
                ->VirtualProperty("NormalShadowBias", "GetNormalShadowBias", "SetNormalShadowBias")
                ->VirtualProperty("ShadowmapMaxSize", "GetShadowmapMaxSize", "SetShadowmapMaxSize")
                ->VirtualProperty("ShadowFilterMethod", "GetShadowFilterMethod", "SetShadowFilterMethod")
                ->VirtualProperty("FilteringSampleCount", "GetFilteringSampleCount", "SetFilteringSampleCount")
                ->VirtualProperty("EsmExponent", "GetEsmExponent", "SetEsmExponent")
                ->VirtualProperty("ShadowCachingMode", "GetShadowCachingMode", "SetShadowCachingMode")

                ->VirtualProperty("AffectsGI", "GetAffectsGI", "SetAffectsGI")
                ->VirtualProperty("AffectsGIFactor", "GetAffectsGIFactor", "SetAffectsGIFactor")
                ->VirtualProperty("LightingChannelMask", "GetLightingChannelMask", "SetLightingChannelMask");
        }
    }

    void AreaLightComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AreaLightService"));
    }

    void AreaLightComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AreaLightService"));
    }
    
    void AreaLightComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("ShapeService"));
    }

    AreaLightComponentController::AreaLightComponentController(const AreaLightComponentConfig& config)
        : m_configuration(config)
    {
    }

    void AreaLightComponentController::Activate(EntityId entityId)
    {
        m_entityId = entityId;
        
        // Used to determine which features are supported.
        m_configuration.m_shapeType = 0;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(m_configuration.m_shapeType, m_entityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetShapeType);

        VerifyLightTypeAndShapeComponent();
        CreateLightShapeDelegate();

        if (m_configuration.RequiresShapeComponent() && m_lightShapeDelegate == nullptr)
        {
            AZ_Error("AreaLightComponentController", false, "AreaLightComponentController activated without having required shape component.");
        }

        AreaLightRequestBus::Handler::BusConnect(m_entityId);

        ConfigurationChanged();
    }

    void AreaLightComponentController::Deactivate()
    {
        AreaLightRequestBus::Handler::BusDisconnect(m_entityId);
        m_lightShapeDelegate.reset();
    }

    void AreaLightComponentController::SetConfiguration(const AreaLightComponentConfig& config)
    {
        m_configuration = config;

        VerifyLightTypeAndShapeComponent();
        ConfigurationChanged();
    }

    const AreaLightComponentConfig& AreaLightComponentController::GetConfiguration() const
    {
        m_configuration.m_cacheShadows = m_configuration.m_shadowCachingMode == AreaLightComponentConfig::ShadowCachingMode::UpdateOnChange;
        return m_configuration;
    }

    void AreaLightComponentController::SetVisibiliy(bool isVisible)
    {
        m_isVisible = isVisible;
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetVisibility(m_isVisible);
            if (m_isVisible)
            {
                // If the light is made visible, make sure to apply the configuration so all properties as set correctly.
                ConfigurationChanged();
            }
        }
    }
    
    void AreaLightComponentController::VerifyLightTypeAndShapeComponent()
    {
        constexpr Crc32 SphereShapeTypeId = AZ_CRC_CE("Sphere");
        constexpr Crc32 DiskShapeTypeId = AZ_CRC_CE("DiskShape");
        constexpr Crc32 CapsuleShapeTypeId = AZ_CRC_CE("Capsule");
        constexpr Crc32 QuadShapeTypeId = AZ_CRC_CE("QuadShape");
        constexpr Crc32 PoylgonShapeTypeId = AZ_CRC_CE("PolygonPrism");

        if (m_configuration.m_lightType == AreaLightComponentConfig::LightType::Unknown)
        {
            // Light type is unknown, see if it can be determined from a shape component.
            switch (m_configuration.m_shapeType)
            {
            case SphereShapeTypeId:
                m_configuration.m_lightType = AreaLightComponentConfig::LightType::Sphere;
                break;
            case DiskShapeTypeId:
                m_configuration.m_lightType = AreaLightComponentConfig::LightType::SpotDisk;
                break;
            case CapsuleShapeTypeId:
                m_configuration.m_lightType = AreaLightComponentConfig::LightType::Capsule;
                break;
            case QuadShapeTypeId:
                m_configuration.m_lightType = AreaLightComponentConfig::LightType::Quad;
                break;
            case PoylgonShapeTypeId:
                m_configuration.m_lightType = AreaLightComponentConfig::LightType::Polygon;
                break;
            default:
                break; // Light type can't be deduced. 
            }
        }
        else if (m_configuration.m_shapeType == Crc32(0))
        {
            AZ_Error("AreaLightComponentController", !m_configuration.RequiresShapeComponent(), "The light type used on this area light requires a corresponding shape component");
        }
        else
        {
            // Validate the the light type matches up with shape type if the light type is an area light.
            AZ_Error("AreaLightComponentController",
                !(m_configuration.m_lightType == AreaLightComponentConfig::LightType::Sphere && m_configuration.m_shapeType != SphereShapeTypeId),
                "The light type is a sphere, but the shape component is not.");
            AZ_Error("AreaLightComponentController",
                !(m_configuration.m_lightType == AreaLightComponentConfig::LightType::SpotDisk && m_configuration.m_shapeType != DiskShapeTypeId),
                "The light type is a disk, but the shape component is not.");
            AZ_Error("AreaLightComponentController",
                !(m_configuration.m_lightType == AreaLightComponentConfig::LightType::Capsule && m_configuration.m_shapeType != CapsuleShapeTypeId),
                "The light type is a capsule, but the shape component is not.");
            AZ_Error("AreaLightComponentController",
                !(m_configuration.m_lightType == AreaLightComponentConfig::LightType::Quad && m_configuration.m_shapeType != QuadShapeTypeId),
                "The light type is a quad, but the shape component is not.");
            AZ_Error("AreaLightComponentController",
                !(m_configuration.m_lightType == AreaLightComponentConfig::LightType::Polygon && m_configuration.m_shapeType != PoylgonShapeTypeId),
                "The light type is a polygon, but the shape component is not.");
        }

        if (m_configuration.m_lightType == AreaLightComponentConfig::LightType::SimpleSpot)
        {
            m_configuration.m_enableShutters = true; // Simple spot always has shutters.
        }
    }

    void AreaLightComponentController::ConfigurationChanged()
    {
        m_configuration.m_shadowCachingMode = m_configuration.m_cacheShadows
            ? AreaLightComponentConfig::ShadowCachingMode::UpdateOnChange
            : AreaLightComponentConfig::ShadowCachingMode::NoCaching;

        ChromaChanged();
        IntensityChanged();
        AttenuationRadiusChanged();
        ShuttersChanged();
        ShadowsChanged();
        LightingChannelMaskChanged();

        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetLightEmitsBothDirections(m_configuration.m_lightEmitsBothDirections);
            m_lightShapeDelegate->SetUseFastApproximation(m_configuration.m_useFastApproximation);
            m_lightShapeDelegate->SetAffectsGI(m_configuration.m_affectsGI);
            m_lightShapeDelegate->SetAffectsGIFactor(m_configuration.m_affectsGIFactor);
            AZ::Data::Instance<AZ::RPI::StreamingImage> image = nullptr;
            if (m_configuration.m_goboImageAsset.GetId().IsValid())
            {
                image = AZ::RPI::StreamingImage::FindOrCreate(m_configuration.m_goboImageAsset);
            }
            m_lightShapeDelegate->SetGoboTexture(image);
        }
    }

    void AreaLightComponentController::IntensityChanged()
    {
        AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnColorOrIntensityChanged, m_configuration.m_color, m_configuration.m_intensity);

        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetPhotometricUnit(m_configuration.m_intensityMode);
            m_lightShapeDelegate->SetIntensity(m_configuration.m_intensity);
        }

        if (m_configuration.m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic)
        {
            AttenuationRadiusChanged();
        }
    }

    void AreaLightComponentController::ChromaChanged()
    {
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetChroma(m_configuration.m_color);
        }
    }

    void AreaLightComponentController::AttenuationRadiusChanged()
    {
        if (m_configuration.m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic)
        {
            AutoCalculateAttenuationRadius();
        }
        AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnAttenutationRadiusChanged, m_configuration.m_attenuationRadius);

        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetAttenuationRadius(m_configuration.m_attenuationRadius);
        }
    }
    
    void AreaLightComponentController::ShuttersChanged()
    {
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetEnableShutters(m_configuration.m_enableShutters);
            if (m_configuration.m_enableShutters)
            {
                m_lightShapeDelegate->SetShutterAngles(m_configuration.m_innerShutterAngleDegrees, m_configuration.m_outerShutterAngleDegrees);
            }
        }
    }

    void AreaLightComponentController::ShadowsChanged()
    {
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetEnableShadow(m_configuration.m_enableShadow);
            if (m_configuration.m_enableShadow)
            {
                m_lightShapeDelegate->SetShadowBias(m_configuration.m_bias);
                m_lightShapeDelegate->SetNormalShadowBias(m_configuration.m_normalShadowBias);
                m_lightShapeDelegate->SetShadowmapMaxSize(m_configuration.m_shadowmapMaxSize);
                m_lightShapeDelegate->SetShadowFilterMethod(m_configuration.m_shadowFilterMethod);
                m_lightShapeDelegate->SetFilteringSampleCount(m_configuration.m_filteringSampleCount);
                m_lightShapeDelegate->SetEsmExponent(m_configuration.m_esmExponent);
                m_lightShapeDelegate->SetShadowCachingMode(m_configuration.m_shadowCachingMode);
            }
        }
    }

    void AreaLightComponentController::LightingChannelMaskChanged()
    {
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetLightingChannelMask(m_configuration.m_lightingChannelConfig.GetLightingChannelMask());
        }
    }    

    void AreaLightComponentController::AutoCalculateAttenuationRadius()
    {
        if (m_lightShapeDelegate)
        {
            m_configuration.m_attenuationRadius = m_lightShapeDelegate->CalculateAttenuationRadius(AreaLightComponentConfig::CutoffIntensity);
        }
    }

    const Color& AreaLightComponentController::GetColor() const
    {
        return m_configuration.m_color;
    }

    void AreaLightComponentController::SetColor(const Color& color)
    {
        m_configuration.m_color = color;
        AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnColorChanged, color);
        ChromaChanged();
    }

    bool AreaLightComponentController::GetLightEmitsBothDirections() const
    {
        return m_configuration.m_lightEmitsBothDirections;
    }

    void AreaLightComponentController::SetLightEmitsBothDirections(bool value)
    {
        m_configuration.m_lightEmitsBothDirections = value;
    }

    bool AreaLightComponentController::GetUseFastApproximation() const
    {
        return m_configuration.m_useFastApproximation;
    }

    void AreaLightComponentController::SetUseFastApproximation(bool value)
    {
        m_configuration.m_useFastApproximation = value;
    }

    PhotometricUnit AreaLightComponentController::GetIntensityMode() const
    {
        return m_configuration.m_intensityMode;
    }

    float AreaLightComponentController::GetIntensity() const
    {
        return m_configuration.m_intensity;
    }

    void AreaLightComponentController::SetIntensityAndMode(float intensity, PhotometricUnit intensityMode)
    {
        m_configuration.m_intensityMode = intensityMode;
        m_configuration.m_intensity = intensity;

        AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnIntensityChanged, intensity, intensityMode);
        IntensityChanged();
    }

    void AreaLightComponentController::SetIntensity(float intensity, PhotometricUnit intensityMode)
    {
        AZ_Warning("Main Window", false, "This verion of SetIntensity() is deprecated. Use SetIntensityMode() instead.");

        m_configuration.m_intensityMode = intensityMode;
        m_configuration.m_intensity = intensity;

        AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnIntensityChanged, intensity, intensityMode);
        IntensityChanged();
    }

    void AreaLightComponentController::SetIntensity(float intensity)
    {
        m_configuration.m_intensity = intensity;

        AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnIntensityChanged, intensity, m_configuration.m_intensityMode);
        IntensityChanged();
    }

    float AreaLightComponentController::GetAttenuationRadius() const
    {
        return m_configuration.m_attenuationRadius;
    }

    void AreaLightComponentController::SetAttenuationRadius(float radius)
    {
        m_configuration.m_attenuationRadius = radius;
        m_configuration.m_attenuationRadiusMode = LightAttenuationRadiusMode::Explicit;
        AttenuationRadiusChanged();
    }

    void AreaLightComponentController::SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode)
    {
        m_configuration.m_attenuationRadiusMode = attenuationRadiusMode;
        AttenuationRadiusChanged();
    }

    void AreaLightComponentController::ConvertToIntensityMode(PhotometricUnit intensityMode)
    {
        if (m_lightShapeDelegate && m_lightShapeDelegate->GetPhotometricValue().GetType() != intensityMode)
        {
            m_configuration.m_intensityMode = intensityMode;
            m_configuration.m_intensity = m_lightShapeDelegate->SetPhotometricUnit(intensityMode);
        }
    }

    float AreaLightComponentController::GetSurfaceArea() const
    {
        return m_lightShapeDelegate ? m_lightShapeDelegate->GetSurfaceArea() : 0.0f;
    }
    
    bool AreaLightComponentController::GetEnableShutters() const
    {
        return m_configuration.m_enableShutters;
    }

    void AreaLightComponentController::SetEnableShutters(bool enabled)
    {
        m_configuration.m_enableShutters = enabled && m_configuration.SupportsShutters();
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetEnableShutters(enabled);
        }
    }

    float AreaLightComponentController::GetInnerShutterAngle() const
    {
        return m_configuration.m_innerShutterAngleDegrees;
    }

    void AreaLightComponentController::SetInnerShutterAngle(float degrees)
    {
        m_configuration.m_innerShutterAngleDegrees = degrees;
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetShutterAngles(m_configuration.m_innerShutterAngleDegrees, m_configuration.m_outerShutterAngleDegrees);
        }
    }

    float AreaLightComponentController::GetOuterShutterAngle() const
    {
        return m_configuration.m_outerShutterAngleDegrees;
    }

    void AreaLightComponentController::SetOuterShutterAngle(float degrees)
    {
        m_configuration.m_outerShutterAngleDegrees = degrees;
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetShutterAngles(m_configuration.m_innerShutterAngleDegrees, m_configuration.m_outerShutterAngleDegrees);
        }
    }

    bool AreaLightComponentController::GetEnableShadow() const
    {
        return m_configuration.m_enableShadow;
    }

    void AreaLightComponentController::SetEnableShadow(bool enabled)
    {
        m_configuration.m_enableShadow = enabled && m_configuration.SupportsShadows();
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetEnableShadow(enabled);
        }
    }
    
    float AreaLightComponentController::GetShadowBias() const
    {
        return m_configuration.m_bias;
    }

    void AreaLightComponentController::SetShadowBias(float bias)
    {
        m_configuration.m_bias = bias;
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetShadowBias(bias);
        }
    }

    void AreaLightComponentController::SetNormalShadowBias(float bias)
    {
        m_configuration.m_normalShadowBias = bias;
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetNormalShadowBias(bias);
        }
    }

    float AreaLightComponentController::GetNormalShadowBias() const
    {
        return m_configuration.m_normalShadowBias;
    }

    ShadowmapSize AreaLightComponentController::GetShadowmapMaxSize() const
    {
        return m_configuration.m_shadowmapMaxSize;
    }

    void AreaLightComponentController::SetShadowmapMaxSize(ShadowmapSize size)
    {
        m_configuration.m_shadowmapMaxSize = size;
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetShadowmapMaxSize(size);
        }
    }

    ShadowFilterMethod AreaLightComponentController::GetShadowFilterMethod() const
    {
        return m_configuration.m_shadowFilterMethod;
    }

    void AreaLightComponentController::SetShadowFilterMethod(ShadowFilterMethod method)
    {
        m_configuration.m_shadowFilterMethod = method;
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetShadowFilterMethod(method);
        }
    }

    uint32_t AreaLightComponentController::GetFilteringSampleCount() const
    {
        return m_configuration.m_filteringSampleCount;
    }

    void AreaLightComponentController::SetFilteringSampleCount(uint32_t count)
    {
        m_configuration.m_filteringSampleCount = static_cast<uint16_t>(count);
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetFilteringSampleCount(count);
        }
    }

    void AreaLightComponentController::HandleDisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay,
        bool isSelected)
    {
        Transform transform = Transform::CreateIdentity();
        TransformBus::EventResult(transform, m_entityId, &TransformBus::Events::GetWorldTM);

        AZ::Vector3 translationOffset = AZ::Vector3::CreateZero();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            translationOffset, m_entityId, &LmbrCentral::ShapeComponentRequests::GetTranslationOffset);

        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->DrawDebugDisplay(
                transform * AZ::Transform::CreateTranslation(translationOffset), m_configuration.m_color, debugDisplay, isSelected);
        }
    }

    float AreaLightComponentController::GetEsmExponent() const
    {
        return m_configuration.m_esmExponent;
    }

    void AreaLightComponentController::SetEsmExponent(float esmExponent)
    {
        m_configuration.m_esmExponent = esmExponent;
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetEsmExponent(esmExponent);
        }
    }

    AreaLightComponentConfig::ShadowCachingMode AreaLightComponentController::GetShadowCachingMode() const
    {
        return m_configuration.m_shadowCachingMode;
    }

    void AreaLightComponentController::SetShadowCachingMode(AreaLightComponentConfig::ShadowCachingMode cachingMode)
    {
        m_configuration.m_shadowCachingMode = cachingMode;
        m_configuration.m_cacheShadows = m_configuration.m_shadowCachingMode == AreaLightComponentConfig::ShadowCachingMode::UpdateOnChange;

        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetShadowCachingMode(cachingMode);
        }
    }

    bool AreaLightComponentController::GetAffectsGI() const
    {
        return m_configuration.m_affectsGI;
    }

    void AreaLightComponentController::SetAffectsGI(bool affectsGI) const
    {
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetAffectsGI(affectsGI);
        }
    }

    float AreaLightComponentController::GetAffectsGIFactor() const
    {
        return m_configuration.m_affectsGIFactor;
    }

    void AreaLightComponentController::SetAffectsGIFactor(float affectsGIFactor) const
    {
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetAffectsGIFactor(affectsGIFactor);
        }
    }

    uint32_t AreaLightComponentController::GetLightingChannelMask() const
    {
        return m_configuration.m_lightingChannelConfig.GetLightingChannelMask();
    }

    void AreaLightComponentController::SetLightingChannelMask(const uint32_t lightingChannelMask)
    {
        m_configuration.m_lightingChannelConfig.SetLightingChannelMask(lightingChannelMask);
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetLightingChannelMask(m_configuration.m_lightingChannelConfig.GetLightingChannelMask());
        }
    }

    AZ::Aabb AreaLightComponentController::GetLocalVisualizationBounds() const
    {
        return m_lightShapeDelegate ? m_lightShapeDelegate->GetLocalVisualizationBounds()
                                    : AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
    }

    void AreaLightComponentController::CreateLightShapeDelegate()
    {
        switch (m_configuration.m_lightType)
        {

        // Simple types
        case AreaLightComponentConfig::LightType::SimplePoint:
            m_lightShapeDelegate = AZStd::make_unique<SimplePointLightDelegate>(m_entityId, m_isVisible);
            break;
        case AreaLightComponentConfig::LightType::SimpleSpot:
            m_lightShapeDelegate = AZStd::make_unique<SimpleSpotLightDelegate>(m_entityId, m_isVisible);
            break;

        // Area light types
        case AreaLightComponentConfig::LightType::Sphere:
        {
            LmbrCentral::SphereShapeComponentRequests* sphereShapeInterface = LmbrCentral::SphereShapeComponentRequestsBus::FindFirstHandler(m_entityId);
            if (sphereShapeInterface)
            {
                m_lightShapeDelegate = AZStd::make_unique<SphereLightDelegate>(sphereShapeInterface, m_entityId, m_isVisible);
            }
            break;
        }
        case AreaLightComponentConfig::LightType::SpotDisk:
        {
            LmbrCentral::DiskShapeComponentRequests* diskShapeInterface = LmbrCentral::DiskShapeComponentRequestBus::FindFirstHandler(m_entityId);
            if (diskShapeInterface)
            {
                m_lightShapeDelegate = AZStd::make_unique<DiskLightDelegate>(diskShapeInterface, m_entityId, m_isVisible);
            }
            break;
        }
        case AreaLightComponentConfig::LightType::Capsule:
        {
            LmbrCentral::CapsuleShapeComponentRequests* capsuleShapeInterface = LmbrCentral::CapsuleShapeComponentRequestsBus::FindFirstHandler(m_entityId);
            if (capsuleShapeInterface)
            {
                m_lightShapeDelegate = AZStd::make_unique<CapsuleLightDelegate>(capsuleShapeInterface, m_entityId, m_isVisible);
            }
            break;
        }
        case AreaLightComponentConfig::LightType::Quad:
        {
            LmbrCentral::QuadShapeComponentRequests* quadShapeInterface = LmbrCentral::QuadShapeComponentRequestBus::FindFirstHandler(m_entityId);
            if (quadShapeInterface)
            {
                m_lightShapeDelegate = AZStd::make_unique<QuadLightDelegate>(quadShapeInterface, m_entityId, m_isVisible);
            }
            break;
        }
        case AreaLightComponentConfig::LightType::Polygon:
        {
            LmbrCentral::PolygonPrismShapeComponentRequests* polyPrismShapeInterface = LmbrCentral::PolygonPrismShapeComponentRequestBus::FindFirstHandler(m_entityId);
            if (polyPrismShapeInterface)
            {
                m_lightShapeDelegate = AZStd::make_unique<PolygonLightDelegate>(polyPrismShapeInterface, m_entityId, m_isVisible);
            }
            break;
        }
        }
        if (m_lightShapeDelegate)
        {
            m_lightShapeDelegate->SetConfig(&m_configuration);
        }
    }

} // namespace AZ::Render
