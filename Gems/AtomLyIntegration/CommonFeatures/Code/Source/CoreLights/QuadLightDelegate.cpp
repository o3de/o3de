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

#include <CoreLights/QuadLightDelegate.h>
#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

namespace AZ
{
    namespace Render
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
            float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
            return sqrt(intensity / lightThreshold);
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

        void QuadLightDelegate::DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
        {
            if (isSelected)
            {
                debugDisplay.SetColor(color);

                // Draw a quad for the attenuation radius
                debugDisplay.DrawWireSphere(transform.GetTranslation(), CalculateAttenuationRadius(AreaLightComponentConfig::CutoffIntensity));
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

    } // namespace Render
} // namespace AZ
