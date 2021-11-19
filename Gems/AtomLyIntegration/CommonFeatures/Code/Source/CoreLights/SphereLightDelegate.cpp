/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/SphereLightDelegate.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Feature/CoreLights/PointLightFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

namespace AZ::Render
{
    SphereLightDelegate::SphereLightDelegate(LmbrCentral::SphereShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible)
        : LightDelegateBase<PointLightFeatureProcessorInterface>(entityId, isVisible)
        , m_shapeBus(shapeBus)
    {
        InitBase(entityId);
    }

    float SphereLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
    {
        // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
        float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
        return sqrt(intensity / lightThreshold);
    }
        
    void SphereLightDelegate::HandleShapeChanged()
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
            GetFeatureProcessor()->SetBulbRadius(GetLightHandle(), GetRadius());
        }
    }

    float SphereLightDelegate::GetSurfaceArea() const
    {
        float radius = GetRadius();
        return 4.0f * Constants::Pi * radius * radius;
    }

    float SphereLightDelegate::GetRadius() const
    {
        return m_shapeBus->GetRadius() * GetTransform().GetUniformScale();
    }

    void SphereLightDelegate::DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
    {
        if (isSelected)
        {
            debugDisplay.SetColor(color);
                
            // Draw a sphere for the attenuation radius
            debugDisplay.DrawWireSphere(transform.GetTranslation(), GetConfig()->m_attenuationRadius);
        }
    }

    void SphereLightDelegate::SetEnableShadow(bool enabled)
    {
        Base::SetEnableShadow(enabled);

        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowsEnabled(GetLightHandle(), enabled);
        }
    }
        
    void SphereLightDelegate::SetShadowBias(float bias)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowBias(GetLightHandle(), bias);
        }
    }

    void SphereLightDelegate::SetShadowmapMaxSize(ShadowmapSize size)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowmapMaxResolution(GetLightHandle(), size);
        }
    }

    void SphereLightDelegate::SetShadowFilterMethod(ShadowFilterMethod method)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowFilterMethod(GetLightHandle(), method);
        }
    }

    void SphereLightDelegate::SetFilteringSampleCount(uint32_t count)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetFilteringSampleCount(GetLightHandle(), static_cast<uint16_t>(count));
        }
    }

    void SphereLightDelegate::SetEsmExponent(float esmExponent)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetEsmExponent(GetLightHandle(), esmExponent);
        }
    }
} // namespace AZ::Render
