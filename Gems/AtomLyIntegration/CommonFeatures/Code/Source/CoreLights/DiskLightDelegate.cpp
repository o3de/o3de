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

#include <CoreLights/DiskLightDelegate.h>
#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        DiskLightDelegate::DiskLightDelegate(LmbrCentral::DiskShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible)
            : LightDelegateBase<DiskLightFeatureProcessorInterface>(entityId, isVisible)
            , m_shapeBus(shapeBus)
        {
            InitBase(entityId);
        }
                
        void DiskLightDelegate::SetLightEmitsBothDirections(bool lightEmitsBothDirections)
        {
            if (GetLightHandle().IsValid())
            {
                GetFeatureProcessor()->SetLightEmitsBothDirections(GetLightHandle(), lightEmitsBothDirections);
            }
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
            return m_shapeBus->GetRadius() * GetTransform().GetScale().GetMaxElement();
        }

        void DiskLightDelegate::DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
        {
            if (isSelected)
            {
                debugDisplay.SetColor(color);

                // Draw a disk for the attenuation radius
                debugDisplay.DrawWireSphere(transform.GetTranslation(), CalculateAttenuationRadius(AreaLightComponentConfig::CutoffIntensity));
            }
        }
    } // namespace Render
} // namespace AZ
