/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <Atom/Feature/SkyAtmosphere/SkyAtmosphereFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/SkyAtmosphere/SkyAtmosphereComponentConfig.h>

namespace AZ::Render
{
    class SkyAtmosphereComponentController final
        : public TransformNotificationBus::Handler
    {
    public:
        friend class EditorSkyAtmosphereComponent;

        AZ_TYPE_INFO(AZ::Render::SkyAtmosphereComponentController, "{CB3DC903-ADAD-4127-9740-2D28AA890C2F}");
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        SkyAtmosphereComponentController() = default;
        SkyAtmosphereComponentController(const SkyAtmosphereComponentConfig& config);

        void Activate(EntityId entityId);
        void Deactivate();
        void SetConfiguration(const SkyAtmosphereComponentConfig& config);
        const SkyAtmosphereComponentConfig& GetConfiguration() const;

    private:
        AZ_DISABLE_COPY(SkyAtmosphereComponentController);

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        TransformInterface* m_transformInterface = nullptr;
        SkyAtmosphereFeatureProcessorInterface* m_featureProcessorInterface = nullptr;
        SkyAtmosphereFeatureProcessorInterface::AtmosphereId m_atmosphereId;
        SkyAtmosphereComponentConfig m_configuration;
        EntityId m_entityId;
    };
} // namespace AZ::Render
