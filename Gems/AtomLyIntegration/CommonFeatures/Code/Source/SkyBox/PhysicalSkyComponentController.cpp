/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyBox/PhysicalSkyComponentController.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        void PhysicalSkyComponentController::Reflect(ReflectContext* context)
        {
            PhysicalSkyComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PhysicalSkyComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &PhysicalSkyComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PhysicalSkyRequestBus>("PhysicalSkyRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Event("SetTurbidity", &PhysicalSkyRequestBus::Events::SetTurbidity)
                    ->Event("GetTurbidity", &PhysicalSkyRequestBus::Events::GetTurbidity)
                    ->Event("SetSunRadiusFactor", &PhysicalSkyRequestBus::Events::SetSunRadiusFactor)
                    ->Event("GetSunRadiusFactor", &PhysicalSkyRequestBus::Events::GetSunRadiusFactor)
                    ->Event("SetSkyIntensity", static_cast<void(PhysicalSkyRequestBus::Events::*)(float)>(&PhysicalSkyRequestBus::Events::SetSkyIntensity))
                    ->Event("GetSkyIntensity", static_cast<float(PhysicalSkyRequestBus::Events::*)()>(&PhysicalSkyRequestBus::Events::GetSkyIntensity))
                    ->Event("SetSunIntensity", static_cast<void(PhysicalSkyRequestBus::Events::*)(float)>(&PhysicalSkyRequestBus::Events::SetSunIntensity))
                    ->Event("GetSunIntensity", static_cast<float(PhysicalSkyRequestBus::Events::*)()>(&PhysicalSkyRequestBus::Events::GetSunIntensity))
                    ->VirtualProperty("Turbidity", "GetTurbidity", "SetTurbidity")
                    ->VirtualProperty("SunRadiusFactor", "GetSunRadiusFactor", "SetSunRadiusFactor")
                    ->VirtualProperty("SkyIntensity", "GetSkyIntensity", "SetSkyIntensity")
                    ->VirtualProperty("SunIntensity", "GetSunIntensity", "SetSunIntensity")
                    ;
            }
        }

        void PhysicalSkyComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SkyBoxService"));
        }

        void PhysicalSkyComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SkyBoxService"));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        void PhysicalSkyComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        PhysicalSkyComponentController::PhysicalSkyComponentController(const PhysicalSkyComponentConfig& config)
            : m_configuration(config)
        {
        }

        void PhysicalSkyComponentController::Activate(EntityId entityId)
        {
            m_featureProcessorInterface = RPI::Scene::GetFeatureProcessorForEntity<SkyBoxFeatureProcessorInterface>(entityId);

            // only activate if there is no other skybox activate
            if (!m_featureProcessorInterface->IsEnabled())
            {
                m_featureProcessorInterface->SetSkyboxMode(SkyBoxMode::PhysicalSky);
                m_featureProcessorInterface->Enable(true);

                m_entityId = entityId;
                m_skyPhotometricValue = PhotometricValue(m_configuration.m_skyIntensity, Color::CreateOne(), m_configuration.m_intensityMode);
                m_sunPhotometricValue = PhotometricValue(m_configuration.m_sunIntensity, Color::CreateOne(), m_configuration.m_intensityMode);

                SetTurbidity(m_configuration.m_turbidity);
                SetSunRadiusFactor(m_configuration.m_sunRadiusFactor);
                SetSkyIntensity(m_configuration.m_skyIntensity, m_configuration.m_intensityMode);
                SetSunIntensity(m_configuration.m_sunIntensity, m_configuration.m_intensityMode);

                m_transformInterface = TransformBus::FindFirstHandler(m_entityId);
                AZ_Assert(m_transformInterface, "Unable to attach to a TransformBus handler. Entity transform will not affect to skybox.");

                const AZ::Transform& transform = m_transformInterface ? m_transformInterface->GetWorldTM() : Transform::Identity();
                m_featureProcessorInterface->SetSunPosition(GetSunTransform(transform));
                m_featureProcessorInterface->SetFogSettings(m_configuration.m_skyBoxFogSettings);

                PhysicalSkyRequestBus::Handler::BusConnect(m_entityId);
                SkyBoxFogRequestBus::Handler::BusConnect(m_entityId);
                TransformNotificationBus::Handler::BusConnect(m_entityId);

                m_isActive = true;
            }
            else
            {
                m_featureProcessorInterface = nullptr;
                AZ_Warning("PhysicalSkyComponentController", false, "There is already another HDRi Skybox or Physical Sky component in the scene!");
            }
        }

        void PhysicalSkyComponentController::Deactivate()
        {
            // Run deactivate if this skybox is activate
            if (m_isActive)
            {
                PhysicalSkyRequestBus::Handler::BusDisconnect();
                SkyBoxFogRequestBus::Handler::BusDisconnect();
                TransformNotificationBus::Handler::BusDisconnect();

                m_featureProcessorInterface->Enable(false);
                m_featureProcessorInterface = nullptr;

                m_transformInterface = nullptr;
                m_isActive = false;
            }
        }

        void PhysicalSkyComponentController::SetConfiguration(const PhysicalSkyComponentConfig& config)
        {
            m_configuration = config;
        }

        const PhysicalSkyComponentConfig& PhysicalSkyComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void PhysicalSkyComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            m_featureProcessorInterface->SetSunPosition(GetSunTransform(world));
        }

        void PhysicalSkyComponentController::SetTurbidity(int turbidity)
        {
            m_configuration.m_turbidity = turbidity;
            m_featureProcessorInterface->SetTurbidity(turbidity);
        }

        int PhysicalSkyComponentController::GetTurbidity()
        {
            return m_configuration.m_turbidity;
        }

        void PhysicalSkyComponentController::SetSunRadiusFactor(float factor)
        {
            m_configuration.m_sunRadiusFactor = factor;
            m_featureProcessorInterface->SetSunRadiusFactor(factor);
        }

        float PhysicalSkyComponentController::GetSunRadiusFactor()
        {
            return m_configuration.m_sunRadiusFactor;
        }

        void PhysicalSkyComponentController::SetSkyIntensity(float intensity, PhotometricUnit unit)
        {
            m_skyPhotometricValue.ConvertToPhotometricUnit(unit);
            m_skyPhotometricValue.SetIntensity(intensity);
            m_configuration.m_skyIntensity = intensity;
            m_featureProcessorInterface->SetSkyIntensity(intensity, unit);
        }

        void PhysicalSkyComponentController::SetSkyIntensity(float intensity)
        {
            m_skyPhotometricValue.SetIntensity(intensity);
            m_configuration.m_skyIntensity = intensity;
            m_featureProcessorInterface->SetSkyIntensity(intensity, m_skyPhotometricValue.GetType());
        }

        float PhysicalSkyComponentController::GetSkyIntensity(PhotometricUnit unit)
        {
            m_skyPhotometricValue.ConvertToPhotometricUnit(unit);
            m_configuration.m_skyIntensity = m_skyPhotometricValue.GetIntensity();
            return m_configuration.m_skyIntensity;
        }

        float PhysicalSkyComponentController::GetSkyIntensity()
        {
            return m_configuration.m_skyIntensity;
        }

        void PhysicalSkyComponentController::SetSunIntensity(float intensity, PhotometricUnit unit)
        {
            m_sunPhotometricValue.ConvertToPhotometricUnit(unit);
            m_sunPhotometricValue.SetIntensity(intensity);
            m_configuration.m_sunIntensity = intensity;
            m_featureProcessorInterface->SetSunIntensity(intensity, unit);
        }

        void PhysicalSkyComponentController::SetSunIntensity(float intensity)
        {
            m_sunPhotometricValue.SetIntensity(intensity);
            m_configuration.m_sunIntensity = intensity;
            m_featureProcessorInterface->SetSunIntensity(intensity, m_sunPhotometricValue.GetType());
        }

        float PhysicalSkyComponentController::GetSunIntensity(PhotometricUnit unit)
        {
            m_sunPhotometricValue.ConvertToPhotometricUnit(unit);
            m_configuration.m_sunIntensity = m_sunPhotometricValue.GetIntensity();
            return m_configuration.m_sunIntensity;
        }

        float PhysicalSkyComponentController::GetSunIntensity()
        {
            return m_configuration.m_sunIntensity;
        }

        SunPosition PhysicalSkyComponentController::GetSunTransform(const AZ::Transform& world)
        {
            Transform worldNoScale = world;
            worldNoScale.ExtractUniformScale();
            AZ::Vector3 sunPositionAtom = worldNoScale.TransformVector(AZ::Vector3(0, -1, 0)); // transform Sun from default position

            // Convert sun position to Y-up coordinate
            AZ::Vector3 sunPosition;
            sunPosition.SetX(-sunPositionAtom.GetY());
            sunPosition.SetY(sunPositionAtom.GetZ());
            sunPosition.SetZ(sunPositionAtom.GetX());

            return SunPosition(atan2(sunPosition.GetZ(), sunPosition.GetX()), asin(sunPosition.GetY()));
        }

        void PhysicalSkyComponentController::SetEnabled(bool enable)
        {
            m_configuration.m_skyBoxFogSettings.m_enable = enable;
            m_featureProcessorInterface->SetFogEnabled(enable);
        }

        bool PhysicalSkyComponentController::IsEnabled() const
        {
            return m_configuration.m_skyBoxFogSettings.m_enable;
        }

        void PhysicalSkyComponentController::SetColor(const AZ::Color& color)
        {
            m_configuration.m_skyBoxFogSettings.m_color = color;
            m_featureProcessorInterface->SetFogColor(color);
        }

        const AZ::Color& PhysicalSkyComponentController::GetColor() const
        {
            return m_configuration.m_skyBoxFogSettings.m_color;
        }

        void PhysicalSkyComponentController::SetTopHeight(float topHeight)
        {
            m_configuration.m_skyBoxFogSettings.m_topHeight = topHeight;
            m_featureProcessorInterface->SetFogTopHeight(topHeight);
        }

        float PhysicalSkyComponentController::GetTopHeight() const
        {
            return m_configuration.m_skyBoxFogSettings.m_topHeight;
        }

        void PhysicalSkyComponentController::SetBottomHeight(float bottomHeight)
        {
            m_configuration.m_skyBoxFogSettings.m_bottomHeight = bottomHeight;
            m_featureProcessorInterface->SetFogBottomHeight(bottomHeight);
        }

        float PhysicalSkyComponentController::GetBottomHeight() const
        {
            return m_configuration.m_skyBoxFogSettings.m_bottomHeight;
        }
    }
}
