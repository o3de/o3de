/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            float scale = GetTransform().GetUniformScale();
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

                float scale = GetTransform().GetUniformScale();
                float radius = m_shapeBus->GetRadius();
                GetFeatureProcessor()->SetCapsuleRadius(GetLightHandle(), scale * radius);
            }
        }

        float CapsuleLightDelegate::GetSurfaceArea() const
        {
            float scale = GetTransform().GetUniformScale();
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
                float radius = GetConfig()->m_attenuationRadius;

                // Add on the caps for the attenuation radius
                float scale = GetTransform().GetUniformScale();
                float height = m_shapeBus->GetHeight() * scale;

                debugDisplay.SetColor(color);
                debugDisplay.DrawWireCapsule(transform.GetTranslation(), transform.GetBasisZ(), radius, height);
            }
        }

        float CapsuleLightDelegate::GetInteriorHeight() const
        {
            return m_shapeBus->GetHeight() - m_shapeBus->GetRadius() * 2.0f;
        }

        void CapsuleLightDelegate::SetAffectsGI(bool affectsGI)
        {
            if (GetLightHandle().IsValid())
            {
                GetFeatureProcessor()->SetAffectsGI(GetLightHandle(), affectsGI);
            }
        }

        void CapsuleLightDelegate::SetAffectsGIFactor(float affectsGIFactor)
        {
            if (GetLightHandle().IsValid())
            {
                GetFeatureProcessor()->SetAffectsGIFactor(GetLightHandle(), affectsGIFactor);
            }
        }
    } // namespace Render
} // namespace AZ
