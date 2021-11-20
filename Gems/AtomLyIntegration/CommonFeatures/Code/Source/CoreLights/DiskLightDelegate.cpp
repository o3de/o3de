/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/DiskLightDelegate.h>
#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

namespace AZ::Render
{
    DiskLightDelegate::DiskLightDelegate(LmbrCentral::DiskShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible)
        : LightDelegateBase<DiskLightFeatureProcessorInterface>(entityId, isVisible)
        , m_shapeBus(shapeBus)
    {
        InitBase(entityId);
    }

    float DiskLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
    {
        // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
        float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
        return sqrt(intensity / lightThreshold);
    }

    void DiskLightDelegate::HandleShapeChanged()
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
            GetFeatureProcessor()->SetDirection(GetLightHandle(), m_shapeBus->GetNormal());
            GetFeatureProcessor()->SetDiskRadius(GetLightHandle(), GetRadius());
        }
    }

    float DiskLightDelegate::GetSurfaceArea() const
    {
        float radius = GetRadius();
        return Constants::Pi * radius * radius;
    }

    float DiskLightDelegate::GetRadius() const
    {
        return m_shapeBus->GetRadius() * GetTransform().GetUniformScale();
    }

    void DiskLightDelegate::DrawDebugDisplay(const Transform& transform, [[maybe_unused]]const Color&, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
    {
        debugDisplay.PushMatrix(transform);
        const float radius = GetConfig()->m_attenuationRadius;
        const float shapeRadius = m_shapeBus->GetRadius();

        auto DrawConicalFrustum = [&debugDisplay](uint32_t numRadiusLines, const Color& color, float brightness, float topRadius, float bottomRadius, float height)
        {
            const Color displayColor = Color(color.GetAsVector3() * brightness);
            debugDisplay.SetColor(displayColor);
            debugDisplay.DrawWireDisk(Vector3(0.0, 0.0, height), Vector3::CreateAxisZ(), bottomRadius);

            for (uint32_t i = 0; i < numRadiusLines; ++i)
            {
                float radiusLineAngle = float(i) / numRadiusLines * Constants::TwoPi;
                float cosAngle = cos(radiusLineAngle);
                float sinAngle = sin(radiusLineAngle);
                debugDisplay.DrawLine(
                    Vector3(cosAngle * topRadius, sinAngle * topRadius, 0),
                    Vector3(cosAngle * bottomRadius,sinAngle * bottomRadius, height)
                );
            }
        };

        const Color coneColor = isSelected ? Color::CreateOne() : Color(0.0f, 0.75f, 0.75f, 1.0);
        const uint32_t innerConeLines = 8;
        float innerRadians, outerRadians;
        if (GetConfig()->m_enableShutters)
        {   // With shutters enabled, draw inner and outer debug display frustums
            innerRadians = DegToRad(GetConfig()->m_innerShutterAngleDegrees);
            outerRadians = DegToRad(GetConfig()->m_outerShutterAngleDegrees);

            // Draw a cone using the cone angle and attenuation radius
            innerRadians = GetMin(innerRadians, outerRadians);

            float coneRadiusOuter = sin(outerRadians) * radius;
            float coneHeightOuter = cos(outerRadians) * radius;

            // Outer cone frustum 'faded' debug cone
            const uint32_t outerConeLines = 9;
            DrawConicalFrustum(outerConeLines, coneColor, 0.75f, shapeRadius, shapeRadius + coneRadiusOuter, coneHeightOuter);
        }
        else
        {   // Generic debug display frustum 
            const float coneAngle = 25.0f;
            innerRadians = DegToRad(coneAngle);   // 25 degrees debug display
        }

        // Inner cone frustum
        float coneRadiusInner = sin(innerRadians) * radius;
        float coneHeightInner = cos(innerRadians) * radius;
        DrawConicalFrustum(innerConeLines, coneColor, 1.0f, shapeRadius, shapeRadius + coneRadiusInner, coneHeightInner);

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

    
} // namespace AZ::Render
