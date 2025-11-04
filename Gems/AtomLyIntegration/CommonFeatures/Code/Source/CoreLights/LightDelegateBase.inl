/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
            TransformBus::EventResult(entityTransform, entityId, &TransformBus::Events::GetWorldTM);
            m_transform = ComputeOverallTransform(entityTransform);

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
        void LightDelegateBase<FeatureProcessorType>::SetConfig(const AreaLightComponentConfig* config)
        {
            m_componentConfig = config;
        }

        template <typename FeatureProcessorType>
        void LightDelegateBase<FeatureProcessorType>::SetChroma(const Color& color)
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
        void LightDelegateBase<FeatureProcessorType>::OnShapeChanged([[maybe_unused]] ShapeChangeReasons changeReason)
        {
            AZ_Assert(m_shapeBus, "OnShapeChanged called without a shape bus present.");
            // need to update the overall transform whether the change reason is TransformChanged or ShapeChanged, because changes to the
            // translation offset trigger the event with ShapeChanged 
            Aabb aabb; // unused, but required for GetTransformAndLocalBounds()
            AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
            m_shapeBus->GetTransformAndLocalBounds(entityTransform, aabb);
            m_transform = ComputeOverallTransform(entityTransform);
            m_photometricValue.SetArea(GetSurfaceArea());
            if (m_lightHandle.IsValid())
            {
                m_featureProcessor->SetRgbIntensity(m_lightHandle, m_photometricValue.GetCombinedRgb<FeatureProcessorType::PhotometricUnitType>());
            }
            HandleShapeChanged();
        }
        
        template <typename FeatureProcessorType>
        void LightDelegateBase<FeatureProcessorType>::OnTransformChanged(const Transform& /*local*/, const Transform& world)
        {
            m_transform = ComputeOverallTransform(world);
            HandleShapeChanged();
        }

        template <typename FeatureProcessorType>
        AZ::Transform LightDelegateBase<FeatureProcessorType>::ComputeOverallTransform(const Transform& world)
        {
            const AZ::Vector3 translationOffset = m_shapeBus ? m_shapeBus->GetTranslationOffset() : AZ::Vector3::CreateZero();
            return world * AZ::Transform::CreateTranslation(translationOffset);
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
                if (m_shapeBus)
                {
                    // For lights that get their transform from the shape bus, force an OnShapeChanged to update the transform.
                    OnShapeChanged(ShapeChangeReasons::TransformChanged);
                }
                else
                {
                    // OnShapeChanged() already calls this for delegates with a shape bus
                    HandleShapeChanged();
                }
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

        template<typename FeatureProcessorType>
        void LightDelegateBase<FeatureProcessorType>::SetLightingChannelMask(uint32_t lightingChannelMask)
        {
            if (m_lightHandle.IsValid())
            {
                m_featureProcessor->SetLightingChannelMask(m_lightHandle, lightingChannelMask);
            }
        }
    }
}
