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

    void SimpleSpotLightDelegate::DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
    {
        if (isSelected)
        {
            debugDisplay.SetColor(color);

            // Draw a cone for the cone angle and attenuation radius
            debugDisplay.DrawCone(transform.GetTranslation(), transform.GetBasisX(), CalculateAttenuationRadius(AreaLightComponentConfig::CutoffIntensity), false);
        }
    }
} // namespace AZ::Render
