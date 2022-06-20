/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/SimpleSpotLightDelegate.h>
#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

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
            GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
            GetFeatureProcessor()->SetDirection(GetLightHandle(), GetTransform().GetBasisZ());
        }
    }

    float SimpleSpotLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
    {
        // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
        float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
        return sqrt(intensity / lightThreshold);
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
    
    void SimpleSpotLightDelegate::DrawDebugDisplay(const Transform& transform, const Color& /*color*/, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
    {
        if (isSelected)
        {
            float innerRadians = DegToRad(GetConfig()->m_innerShutterAngleDegrees);
            float outerRadians = DegToRad(GetConfig()->m_outerShutterAngleDegrees);
            float radius = GetConfig()->m_attenuationRadius;

            // Draw a cone using the cone angle and attenuation radius
            innerRadians = GetMin(innerRadians, outerRadians);
            float coneRadiusInner = sin(innerRadians) * radius;
            float coneHeightInner = cos(innerRadians) * radius;
            float coneRadiusOuter = sin(outerRadians) * radius;
            float coneHeightOuter = cos(outerRadians) * radius;

            debugDisplay.PushMatrix(transform);

            auto DrawCone = [&debugDisplay](uint32_t numRadiusLines, float radius, float height, float brightness)
            {
                debugDisplay.SetColor(Color(brightness, brightness, brightness, 1.0f));
                debugDisplay.DrawWireDisk(Vector3(0.0, 0.0, height), Vector3::CreateAxisZ(), radius);
            
                for (uint32_t i = 0; i < numRadiusLines; ++i)
                {
                    float radiusLineAngle = float(i) / numRadiusLines * Constants::TwoPi;
                    debugDisplay.DrawLine(Vector3::CreateZero(), Vector3(cos(radiusLineAngle) * radius, sin(radiusLineAngle) * radius, height));
                }
            };
            
            DrawCone(16, coneRadiusInner, coneHeightInner, 1.0f);
            DrawCone(16, coneRadiusOuter, coneHeightOuter, 0.65f);

            debugDisplay.PopMatrix();
        }
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
} // namespace AZ::Render
