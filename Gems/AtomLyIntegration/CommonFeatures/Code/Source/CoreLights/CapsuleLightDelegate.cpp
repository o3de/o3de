/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/CoreLights/CapsuleLightFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>
#include <CoreLights/CapsuleLightDelegate.h>

namespace AZ::Render
{
    CapsuleLightDelegate::CapsuleLightDelegate(LmbrCentral::CapsuleShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible)
        : LightDelegateBase<CapsuleLightFeatureProcessorInterface>(entityId, isVisible)
        , m_capsuleShapeBus(shapeBus)
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

        float radius = Sqrt(((-h2 / 4.0f) + Sqrt((h4 / 16.0f) + (4.0f / t2))) / 2.0f);
        float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Candela);

        return radius * intensity;
    }

    void CapsuleLightDelegate::HandleShapeChanged()
    {
        if (GetLightHandle().IsValid())
        {
            const auto endpoints = m_capsuleShapeBus->GetCapsulePoints();
            GetFeatureProcessor()->SetCapsuleLineSegment(GetLightHandle(), endpoints.m_begin, endpoints.m_end);

            float scale = GetTransform().GetUniformScale();
            float radius = m_capsuleShapeBus->GetRadius();
            GetFeatureProcessor()->SetCapsuleRadius(GetLightHandle(), scale * radius);
        }
    }

    float CapsuleLightDelegate::GetSurfaceArea() const
    {
        float scale = GetTransform().GetUniformScale();
        float radius = m_capsuleShapeBus->GetRadius();
        float capsArea = 4.0f * Constants::Pi * radius * radius; // both caps make a sphere
        float sideArea = 2.0f * Constants::Pi * radius * GetInteriorHeight(); // cylindrical area of capsule
        return (capsArea + sideArea) * scale * scale;
    }

    void CapsuleLightDelegate::DrawDebugDisplay(
        const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
    {
        if (isSelected)
        {
            const auto [radius, height] = CalculateCapsuleVisualizationDimensions();
            debugDisplay.SetColor(color);
            debugDisplay.DrawWireCapsule(
                transform.GetTranslation(), transform.GetBasisZ(), radius, AZ::GetMax(height - 2.0f * radius, 0.0f));
        }
    }

    float CapsuleLightDelegate::GetInteriorHeight() const
    {
        return m_capsuleShapeBus->GetHeight() - m_capsuleShapeBus->GetRadius() * 2.0f;
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

    Aabb CapsuleLightDelegate::GetLocalVisualizationBounds() const
    {
        const auto [radius, height] = CalculateCapsuleVisualizationDimensions();
        const AZ::Vector3 translationOffset = m_shapeBus ? m_shapeBus->GetTranslationOffset() : AZ::Vector3::CreateZero();
        return Aabb::CreateFromMinMax(
            AZ::Vector3(-radius, -radius, AZ::GetMin(-radius, -height * 0.5f)) + translationOffset,
            AZ::Vector3(radius, radius, AZ::GetMax(radius, height * 0.5f)) + translationOffset);
    }

    float CapsuleLightDelegate::GetEffectiveSolidAngle() const
    {
        return PhotometricValue::OmnidirectionalSteradians;
    }

    CapsuleLightDelegate::CapsuleVisualizationDimensions CapsuleLightDelegate::CalculateCapsuleVisualizationDimensions() const
    {
        // Attenuation radius shape is just a capsule with the same internal height, but a radius of the attenuation radius.
        const float radius = GetConfig()->m_attenuationRadius;
        const float scale = GetTransform().GetUniformScale();
        const float height = m_capsuleShapeBus->GetHeight() * scale;
        return CapsuleLightDelegate::CapsuleVisualizationDimensions{ radius, height };
    }
} // namespace AZ::Render
