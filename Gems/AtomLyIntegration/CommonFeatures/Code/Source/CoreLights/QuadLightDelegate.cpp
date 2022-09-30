/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>
#include <CoreLights/QuadLightDelegate.h>

namespace AZ::Render
{
    QuadLightDelegate::QuadLightDelegate(LmbrCentral::QuadShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible)
        : LightDelegateBase<QuadLightFeatureProcessorInterface>(entityId, isVisible)
        , m_shapeBus(shapeBus)
    {
        InitBase(entityId);
    }

    void QuadLightDelegate::SetLightEmitsBothDirections(bool lightEmitsBothDirections)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetLightEmitsBothDirections(GetLightHandle(), lightEmitsBothDirections);
        }
    }

    void QuadLightDelegate::SetUseFastApproximation(bool useFastApproximation)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetUseFastApproximation(GetLightHandle(), useFastApproximation);
        }
    }

    float QuadLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
    {
        // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
        const float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
        return Sqrt(intensity / lightThreshold);
    }

    void QuadLightDelegate::HandleShapeChanged()
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
            GetFeatureProcessor()->SetOrientation(GetLightHandle(), m_shapeBus->GetQuadOrientation());
            GetFeatureProcessor()->SetQuadDimensions(GetLightHandle(), GetWidth(), GetHeight());
        }
    }

    float QuadLightDelegate::GetSurfaceArea() const
    {
        return GetWidth() * GetHeight();
    }

    void QuadLightDelegate::DrawDebugDisplay(
        const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
    {
        if (isSelected)
        {
            debugDisplay.SetColor(color);

            // Draw a quad for the attenuation radius
            debugDisplay.DrawWireSphere(transform.GetTranslation(), GetConfig()->m_attenuationRadius);
        }
    }

    float QuadLightDelegate::GetWidth() const
    {
        return m_shapeBus->GetQuadWidth() * GetTransform().GetUniformScale();
    }

    float QuadLightDelegate::GetHeight() const
    {
        return m_shapeBus->GetQuadHeight() * GetTransform().GetUniformScale();
    }

    void QuadLightDelegate::SetAffectsGI(bool affectsGI)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetAffectsGI(GetLightHandle(), affectsGI);
        }
    }

    void QuadLightDelegate::SetAffectsGIFactor(float affectsGIFactor)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetAffectsGIFactor(GetLightHandle(), affectsGIFactor);
        }
    }

    Aabb QuadLightDelegate::GetLocalVisualizationBounds() const
    {
        return Aabb::CreateCenterRadius(Vector3::CreateZero(), GetConfig()->m_attenuationRadius);
    }
} // namespace AZ::Render
