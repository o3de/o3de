/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>
#include <CoreLights/SimpleSpotLightDelegate.h>

namespace AZ::Render
{
    SimpleSpotLightDelegate::SimpleSpotLightDelegate(EntityId entityId, bool isVisible)
        : LightDelegateBase<SimpleSpotLightFeatureProcessorInterface>(entityId, isVisible)
    {
        InitBase(entityId);
    }

    void SimpleSpotLightDelegate::HandleShapeChanged()
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetTransform(GetLightHandle(), GetTransform());
        }
    }

    float SimpleSpotLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
    {
        // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
        float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
        return Sqrt(intensity / lightThreshold);
    }

    float SimpleSpotLightDelegate::GetSurfaceArea() const
    {
        return 0.0f;
    }

    void SimpleSpotLightDelegate::SetShutterAngles(float innerAngleDegrees, float outerAngleDegrees)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetConeAngles(GetLightHandle(), DegToRad(innerAngleDegrees), DegToRad(outerAngleDegrees));
        }
    }

    SimpleSpotLightDelegate::ConeVisualizationDimensions SimpleSpotLightDelegate::CalculateConeVisualizationDimensions(
        const float degrees) const
    {
        const float attenuationRadius = GetConfig()->m_attenuationRadius;
        const float shutterAngleRadians = DegToRad(degrees);
        const float coneRadius = Sin(shutterAngleRadians) * attenuationRadius;
        const float coneHeight = Cos(shutterAngleRadians) * attenuationRadius;
        return ConeVisualizationDimensions{ coneRadius, coneHeight };
    }

    void SimpleSpotLightDelegate::DrawDebugDisplay(
        const Transform& transform,
        [[maybe_unused]] const Color& color,
        AzFramework::DebugDisplayRequests& debugDisplay,
        [[maybe_unused]] bool isSelected) const
    {
        // Draw a cone using the cone angle and attenuation radius
        auto DrawCone = [&debugDisplay](uint32_t numRadiusLines, float radius, float height, const Color& color, float brightness)
        {
            const Color displayColor = Color(color.GetAsVector3() * brightness);
            debugDisplay.SetColor(displayColor);
            debugDisplay.DrawWireDisk(Vector3(0.0, 0.0, height), Vector3::CreateAxisZ(), radius);

            for (uint32_t i = 0; i < numRadiusLines; ++i)
            {
                float radiusLineAngle = float(i) / numRadiusLines * Constants::TwoPi;
                debugDisplay.DrawLine(Vector3::CreateZero(), Vector3(Cos(radiusLineAngle) * radius, Sin(radiusLineAngle) * radius, height));
            }
        };

        debugDisplay.PushMatrix(transform);

        const auto innerCone =
            CalculateConeVisualizationDimensions(GetMin(GetConfig()->m_innerShutterAngleDegrees, GetConfig()->m_outerShutterAngleDegrees));
        const auto outerCone = CalculateConeVisualizationDimensions(GetConfig()->m_outerShutterAngleDegrees);

        const Color coneColor = isSelected ? Color::CreateOne() : Color(0.0f, 0.75f, 0.75f, 1.0f);
        DrawCone(16, innerCone.m_radius, innerCone.m_height, coneColor, 1.0f);
        DrawCone(16, outerCone.m_radius, outerCone.m_height, coneColor, 0.75f);

        debugDisplay.PopMatrix();
    }

    void SimpleSpotLightDelegate::SetAffectsGI(bool affectsGI)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetAffectsGI(GetLightHandle(), affectsGI);
        }
    }

    void SimpleSpotLightDelegate::SetAffectsGIFactor(float affectsGIFactor)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetAffectsGIFactor(GetLightHandle(), affectsGIFactor);
        }
    }

    Aabb SimpleSpotLightDelegate::GetLocalVisualizationBounds() const
    {
        const auto [radius, height] = [this]
        {
            const auto [innerRadius, innerHeight] = CalculateConeVisualizationDimensions(
                GetMin(GetConfig()->m_outerShutterAngleDegrees, GetConfig()->m_innerShutterAngleDegrees));
            const auto [outerRadius, outerHeight] = CalculateConeVisualizationDimensions(GetConfig()->m_outerShutterAngleDegrees);
            return AZStd::pair{ GetMax(innerRadius, outerRadius), GetMax(innerHeight, outerHeight) };
        }();

        return Aabb::CreateFromMinMax(Vector3(-radius, -radius, 0.0f), Vector3(radius, radius, height));
    }

    void SimpleSpotLightDelegate::SetEnableShadow(bool enabled)
    {
        Base::SetEnableShadow(enabled);

        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowsEnabled(GetLightHandle(), enabled);
        }
    }

    void SimpleSpotLightDelegate::SetShadowBias(float bias)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowBias(GetLightHandle(), bias);
        }
    }

    void SimpleSpotLightDelegate::SetNormalShadowBias(float bias)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetNormalShadowBias(GetLightHandle(), bias);
        }
    }

    void SimpleSpotLightDelegate::SetShadowmapMaxSize(ShadowmapSize size)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowmapMaxResolution(GetLightHandle(), size);
        }
    }

    void SimpleSpotLightDelegate::SetShadowFilterMethod(ShadowFilterMethod method)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowFilterMethod(GetLightHandle(), method);
        }
    }

    void SimpleSpotLightDelegate::SetFilteringSampleCount(uint32_t count)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetFilteringSampleCount(GetLightHandle(), static_cast<uint16_t>(count));
        }
    }

    void SimpleSpotLightDelegate::SetEsmExponent(float exponent)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetEsmExponent(GetLightHandle(), exponent);
        }
    }

    void SimpleSpotLightDelegate::SetShadowCachingMode(AreaLightComponentConfig::ShadowCachingMode cachingMode)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetUseCachedShadows(
                GetLightHandle(), cachingMode == AreaLightComponentConfig::ShadowCachingMode::UpdateOnChange);
        }
    }

    void SimpleSpotLightDelegate::SetGoboTexture(AZ::Data::Instance<AZ::RPI::Image> goboTexture)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetGoboTexture(GetLightHandle(), goboTexture);
        }
    }

} // namespace AZ::Render
