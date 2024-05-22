/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>
#include <CoreLights/DiskLightDelegate.h>

namespace AZ::Render
{
    static const float DefaultConeAngleDegrees = 25.0f; // 25 degrees debug display

    DiskLightDelegate::DiskLightDelegate(LmbrCentral::DiskShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible)
        : LightDelegateBase<DiskLightFeatureProcessorInterface>(entityId, isVisible)
        , m_shapeBus(shapeBus)
    {
        InitBase(entityId);
    }

    float DiskLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
    {
        // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
        const float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
        return Sqrt(intensity / lightThreshold);
    }

    void DiskLightDelegate::HandleShapeChanged()
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetDirection(GetLightHandle(), m_shapeBus->GetNormal());
            GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
            GetFeatureProcessor()->SetDiskRadius(GetLightHandle(), GetRadius());
        }
    }

    float DiskLightDelegate::GetSurfaceArea() const
    {
        const float radius = GetRadius();
        return Constants::Pi * radius * radius;
    }

    float DiskLightDelegate::GetRadius() const
    {
        return m_shapeBus->GetRadius() * GetTransform().GetUniformScale();
    }

    DiskLightDelegate::ConeVisualizationDimensions DiskLightDelegate::CalculateConeVisualizationDimensions(const float degrees) const
    {
        const float attenuationRadius = GetConfig()->m_attenuationRadius;
        const float shapeRadius = m_shapeBus->GetRadius();
        const float radians = DegToRad(degrees);
        const float coneRadius = Sin(radians) * attenuationRadius;
        const float coneHeight = Cos(radians) * attenuationRadius;
        return ConeVisualizationDimensions{ shapeRadius, shapeRadius + coneRadius, coneHeight };
    }

    Aabb DiskLightDelegate::GetLocalVisualizationBounds() const
    {
        const auto [radius, height] = [this]
        {
            if (GetConfig()->m_enableShutters)
            {
                const auto [innerTopRadius, innerBottomRadius, innerHeight] = CalculateConeVisualizationDimensions(
                    GetMin(GetConfig()->m_outerShutterAngleDegrees, GetConfig()->m_innerShutterAngleDegrees));
                const auto [outerTopRadius, outerBottomRadius, outerHeight] =
                    CalculateConeVisualizationDimensions(GetConfig()->m_outerShutterAngleDegrees);
                return AZStd::pair{ GetMax(outerBottomRadius, GetMax(innerBottomRadius, GetMax(innerTopRadius, outerTopRadius))),
                                    GetMax(innerHeight, outerHeight) };
            }

            const auto [topRadius, bottomRadius, height] = CalculateConeVisualizationDimensions(DefaultConeAngleDegrees);
            return AZStd::pair{ GetMax(topRadius, bottomRadius), height };
        }();

        return Aabb::CreateFromMinMax(Vector3(-radius, -radius, 0.0f), Vector3(radius, radius, height));
    }

    void DiskLightDelegate::DrawDebugDisplay(
        const Transform& transform,
        [[maybe_unused]] const Color& color,
        AzFramework::DebugDisplayRequests& debugDisplay,
        bool isSelected) const
    {
        debugDisplay.PushMatrix(transform);

        auto DrawConicalFrustum =
            [&debugDisplay](
                uint32_t numRadiusLines, const Color& color, float brightness, float topRadius, float bottomRadius, float height)
        {
            const Color displayColor = Color(color.GetAsVector3() * brightness);
            debugDisplay.SetColor(displayColor);
            debugDisplay.DrawWireDisk(Vector3(0.0f, 0.0f, height), Vector3::CreateAxisZ(), bottomRadius);

            for (uint32_t i = 0; i < numRadiusLines; ++i)
            {
                const float radiusLineAngle = float(i) / numRadiusLines * Constants::TwoPi;
                const float cosAngle = Cos(radiusLineAngle);
                const float sinAngle = Sin(radiusLineAngle);
                debugDisplay.DrawLine(
                    Vector3(cosAngle * topRadius, sinAngle * topRadius, 0.0f),
                    Vector3(cosAngle * bottomRadius, sinAngle * bottomRadius, height));
            }
        };

        const uint32_t innerConeLines = 8;
        const Color coneColor = isSelected ? Color::CreateOne() : Color(0.0f, 0.75f, 0.75f, 1.0f);
        // With shutters enabled, draw inner and outer debug display frustums
        if (GetConfig()->m_enableShutters)
        {
            const auto [innerTopRadius, innerBottomRadius, innerHeight] = CalculateConeVisualizationDimensions(
                GetMin(GetConfig()->m_outerShutterAngleDegrees, GetConfig()->m_innerShutterAngleDegrees));
            const auto [outerTopRadius, outerBottomRadius, outerHeight] =
                CalculateConeVisualizationDimensions(GetConfig()->m_outerShutterAngleDegrees);

            // Outer cone frustum 'faded' debug cone
            const uint32_t outerConeLines = 9;
            // Draw a cone using the cone angle and attenuation radius
            DrawConicalFrustum(innerConeLines, coneColor, 1.0f, innerTopRadius, innerBottomRadius, innerHeight);
            DrawConicalFrustum(outerConeLines, coneColor, 0.75f, outerTopRadius, outerBottomRadius, outerHeight);
        }
        else
        {
            const auto [topRadius, bottomRadius, height] = CalculateConeVisualizationDimensions(DefaultConeAngleDegrees);
            DrawConicalFrustum(innerConeLines, coneColor, 1.0f, topRadius, bottomRadius, height);
        }

        debugDisplay.PopMatrix();
    }

    void DiskLightDelegate::SetEnableShutters(bool enabled)
    {
        Base::SetEnableShutters(enabled);
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetConstrainToConeLight(GetLightHandle(), true);
        }
    }

    void DiskLightDelegate::SetShutterAngles(float innerAngleDegrees, float outerAngleDegrees)
    {
        if (GetShuttersEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetConeAngles(GetLightHandle(), DegToRad(innerAngleDegrees), DegToRad(outerAngleDegrees));
        }
    }

    void DiskLightDelegate::SetEnableShadow(bool enabled)
    {
        Base::SetEnableShadow(enabled);

        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowsEnabled(GetLightHandle(), enabled);
        }
    }

    void DiskLightDelegate::SetShadowBias(float bias)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowBias(GetLightHandle(), bias);
        }
    }

    void DiskLightDelegate::SetNormalShadowBias(float bias)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetNormalShadowBias(GetLightHandle(), bias);
        }
    }

    void DiskLightDelegate::SetShadowmapMaxSize(ShadowmapSize size)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowmapMaxResolution(GetLightHandle(), size);
        }
    }

    void DiskLightDelegate::SetShadowFilterMethod(ShadowFilterMethod method)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowFilterMethod(GetLightHandle(), method);
        }
    }

    void DiskLightDelegate::SetFilteringSampleCount(uint32_t count)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetFilteringSampleCount(GetLightHandle(), static_cast<uint16_t>(count));
        }
    }

    void DiskLightDelegate::SetEsmExponent(float exponent)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetEsmExponent(GetLightHandle(), exponent);
        }
    }

    void DiskLightDelegate::SetShadowCachingMode(AreaLightComponentConfig::ShadowCachingMode cachingMode)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetUseCachedShadows(GetLightHandle(),
                cachingMode == AreaLightComponentConfig::ShadowCachingMode::UpdateOnChange);
        }
    }

    void DiskLightDelegate::SetAffectsGI(bool affectsGI)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetAffectsGI(GetLightHandle(), affectsGI);
        }
    }

    void DiskLightDelegate::SetAffectsGIFactor(float affectsGIFactor)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetAffectsGIFactor(GetLightHandle(), affectsGIFactor);
        }
    }
} // namespace AZ::Render
