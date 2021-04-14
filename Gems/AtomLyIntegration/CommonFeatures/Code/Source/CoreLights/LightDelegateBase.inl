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

#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        template <typename FeatureProcessorType>
        LightDelegateBase<FeatureProcessorType>::LightDelegateBase(EntityId entityId, bool isVisible)
        {
            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<FeatureProcessorType>(entityId);
            AZ_Error("LightDelegateBase", m_featureProcessor, "Unable to find a %s on the scene.", FeatureProcessorType::RTTI_TypeName());

            if (m_featureProcessor && isVisible)
            {
                m_lightHandle = m_featureProcessor->AcquireLight();
            }
        }

        template <typename FeatureProcessorType>
        LightDelegateBase<FeatureProcessorType>::~LightDelegateBase()
        {
            TransformNotificationBus::Handler::BusDisconnect();
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
            if (m_lightHandle.IsValid())
            {
                m_featureProcessor->ReleaseLight(m_lightHandle);
            }
        }

        template <typename FeatureProcessorType>
        void LightDelegateBase<FeatureProcessorType>::InitBase(EntityId entityId)
        {
            m_photometricValue.SetEffectiveSolidAngle(GetEffectiveSolidAngle());
            m_shapeBus = LmbrCentral::ShapeComponentRequestsBus::FindFirstHandler(entityId);
            TransformBus::EventResult(m_transform, entityId, &TransformBus::Events::GetWorldTM);

            if (m_shapeBus != nullptr)
            {
                LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(entityId);
                OnShapeChanged(ShapeChangeReasons::TransformChanged);
            }
            else if (m_lightHandle.IsValid())
            {
                // Only connect to the transform bus if there's no shape bus, otherwise the shape bus handles transforms.
                TransformNotificationBus::Handler::BusConnect(entityId);
                HandleShapeChanged();
                m_featureProcessor->SetRgbIntensity(m_lightHandle, m_photometricValue.GetCombinedRgb<FeatureProcessorType::PhotometricUnitType>());
            }
        }

        template <typename FeatureProcessorType>
        void LightDelegateBase<FeatureProcessorType>::SetChroma(const AZ::Color& color)
        {
            m_photometricValue.SetChroma(color);
            if (m_lightHandle.IsValid())
            {
                m_featureProcessor->SetRgbIntensity(m_lightHandle, m_photometricValue.GetCombinedRgb<FeatureProcessorType::PhotometricUnitType>());
            }
        }

        template <typename FeatureProcessorType>
        void LightDelegateBase<FeatureProcessorType>::SetIntensity(float intensity)
        {
            m_photometricValue.SetIntensity(intensity);
            if (m_lightHandle.IsValid())
            {
                m_featureProcessor->SetRgbIntensity(m_lightHandle, m_photometricValue.GetCombinedRgb<FeatureProcessorType::PhotometricUnitType>());
            }
        }

        template <typename FeatureProcessorType>
        float LightDelegateBase<FeatureProcessorType>::SetPhotometricUnit(PhotometricUnit unit)
        {
            m_photometricValue.SetArea(GetSurfaceArea());
            m_photometricValue.ConvertToPhotometricUnit(unit);
            if (m_lightHandle.IsValid())
            {
                m_featureProcessor->SetRgbIntensity(m_lightHandle, m_photometricValue.GetCombinedRgb<FeatureProcessorType::PhotometricUnitType>());
            }
            return m_photometricValue.GetIntensity();
        }

        template <typename FeatureProcessorType>
        void LightDelegateBase<FeatureProcessorType>::OnShapeChanged(ShapeChangeReasons changeReason)
        {
            if (changeReason == ShapeChangeReasons::TransformChanged)
            {
                AZ::Aabb aabb; // unused, but required for GetTransformAndLocalBounds()
                m_shapeBus->GetTransformAndLocalBounds(m_transform, aabb);
            }
            m_photometricValue.SetArea(GetSurfaceArea());
            if (m_lightHandle.IsValid())
            {
                m_featureProcessor->SetRgbIntensity(m_lightHandle, m_photometricValue.GetCombinedRgb<FeatureProcessorType::PhotometricUnitType>());
            }
            HandleShapeChanged();
        }
        
        template <typename FeatureProcessorType>
        void LightDelegateBase<FeatureProcessorType>::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
        {
            m_transform = world;
            HandleShapeChanged();
        }

        template <typename FeatureProcessorType>
        void LightDelegateBase<FeatureProcessorType>::SetVisibility(bool isVisible)
        {
            if (m_lightHandle.IsValid() && !isVisible)
            {
                // no longer visible, release light handle
                m_featureProcessor->ReleaseLight(m_lightHandle);
            }
            else if (!m_lightHandle.IsValid() && isVisible && m_featureProcessor)
            {
                // now visible, acquire light handle and update values.
                m_lightHandle = m_featureProcessor->AcquireLight();
                OnShapeChanged(ShapeChangeReasons::TransformChanged);
            }
        }

        template <typename FeatureProcessorType>
        void LightDelegateBase<FeatureProcessorType>::SetAttenuationRadius(float radius)
        {
            if (m_lightHandle.IsValid())
            {
                m_featureProcessor->SetAttenuationRadius(m_lightHandle, radius);
            }
        }

    }
}
