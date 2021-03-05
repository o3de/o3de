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

#include <CoreLights/CapsuleLightDelegate.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Feature/CoreLights/CapsuleLightFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        CapsuleLightDelegate::CapsuleLightDelegate(LmbrCentral::CapsuleShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible)
            : LightDelegateBase<CapsuleLightFeatureProcessorInterface>(entityId, isVisible)
            , m_shapeBus(shapeBus)
        {
            InitBase(entityId);
        }

        float CapsuleLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
        {
            // Calculate the radius at which the irradiance will be equal to lightThreshold.

            lightThreshold = AZStd::max<float>(lightThreshold, 0.001f); // prevent divide by zero.

            // This equation is based off of the integration of a line segment against a perpendicular normal pointing at the center of the
            // line segment from some distance away.

            float scale = GetTransform().GetScale().GetMaxElement();
            float h = GetInteriorHeight() * scale;
            float t2 = lightThreshold * lightThreshold;
            float h2 = h * h;
            float h4 = h2 * h2;

            float radius = sqrt(((-h2 / 4.0f) + sqrt((h4 / 16.0f) + (4.0f / t2))) / 2.0f);
            float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Candela);

            return radius * intensity;
        }

        void CapsuleLightDelegate::HandleShapeChanged()
        {
            if (GetLightHandle().IsValid())
            {
                const auto endpoints = m_shapeBus->GetCapsulePoints();
                GetFeatureProcessor()->SetCapsuleLineSegment(GetLightHandle(), endpoints.m_begin, endpoints.m_end);

                float scale = GetTransform().GetScale().GetMaxElement();
                float radius = m_shapeBus->GetRadius();
                GetFeatureProcessor()->SetCapsuleRadius(GetLightHandle(), scale * radius);
            }
        }

        float CapsuleLightDelegate::GetSurfaceArea() const
        {
            float scale = GetTransform().GetScale().GetMaxElement();
            float radius = m_shapeBus->GetRadius();
            float capsArea = 4.0f * Constants::Pi * radius * radius; // both caps make a sphere
            float sideArea = 2.0f * Constants::Pi * radius * GetInteriorHeight(); // cylindrical area of capsule
            return (capsArea + sideArea) * scale * scale;
        }

        void CapsuleLightDelegate::DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
        {
            if (isSelected)
            {
                // Attenuation radius shape is just a capsule with the same internal height, but a radius of the attenuation radius.
                float radius = CalculateAttenuationRadius(AreaLightComponentConfig::CutoffIntensity);

                // Add on the caps for the attenuation radius
                float scale = GetTransform().GetScale().GetMaxElement();
                float height = m_shapeBus->GetHeight() * scale;

                debugDisplay.SetColor(color);
                debugDisplay.DrawWireCapsule(transform.GetTranslation(), transform.GetBasisZ(), radius, height);
            }
        }

        float CapsuleLightDelegate::GetInteriorHeight() const
        {
            return m_shapeBus->GetHeight() - m_shapeBus->GetRadius() * 2.0f;
        }
    } // namespace Render
} // namespace AZ
