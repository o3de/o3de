/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/SimplePointLightDelegate.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Feature/CoreLights/SimplePointLightFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        SimplePointLightDelegate::SimplePointLightDelegate(EntityId entityId, bool isVisible)
            : LightDelegateBase<SimplePointLightFeatureProcessorInterface>(entityId, isVisible)
        {
            InitBase(entityId);
            if (GetLightHandle().IsValid())
            {
                GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
            }
        }
        float SimplePointLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
        {
            // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
            float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
            return sqrt(intensity / lightThreshold);
        }
        
        float SimplePointLightDelegate::GetSurfaceArea() const
        {
            return 0.0f;
        }
        
        void SimplePointLightDelegate::HandleShapeChanged()
        {
            if (GetLightHandle().IsValid())
            {
                GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
            }
        }

        void SimplePointLightDelegate::DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
        {
            if (isSelected)
            {
                debugDisplay.SetColor(color);
                
                // Draw a sphere for the attenuation radius
                debugDisplay.DrawWireSphere(transform.GetTranslation(), GetConfig()->m_attenuationRadius);
            }
        }

        void SimplePointLightDelegate::SetAffectsGI(bool affectsGI)
        {
            if (GetLightHandle().IsValid())
            {
                GetFeatureProcessor()->SetAffectsGI(GetLightHandle(), affectsGI);
            }
        }

        void SimplePointLightDelegate::SetAffectsGIFactor(float affectsGIFactor)
        {
            if (GetLightHandle().IsValid())
            {
                GetFeatureProcessor()->SetAffectsGIFactor(GetLightHandle(), affectsGIFactor);
            }
        }
    } // namespace Render
} // namespace AZ
