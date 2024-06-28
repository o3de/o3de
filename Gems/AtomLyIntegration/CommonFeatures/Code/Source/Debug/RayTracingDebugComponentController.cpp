/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Debug/RayTracingDebugFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Debug/RayTracingDebugComponentController.h>

namespace AZ::Render
{
    void RayTracingDebugComponentController::Reflect(ReflectContext* context)
    {
        RayTracingDebugComponentConfig::Reflect(context);

        if (auto serializeContext{ azrtti_cast<SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugComponentController>()
                ->Version(0)
                ->Field("Configuration", &RayTracingDebugComponentController::m_configuration)
            ;
            // clang-format on
        }
    }

    RayTracingDebugComponentController::RayTracingDebugComponentController(const RayTracingDebugComponentConfig& config)
        : m_configuration{ config }
    {
    }

    void RayTracingDebugComponentController::Activate(EntityId entityId)
    {
        m_entityId = entityId;

        auto fp{ RPI::Scene::GetFeatureProcessorForEntity<RayTracingDebugFeatureProcessorInterface>(m_entityId) };
        if (fp)
        {
            m_rayTracingDebugSettingsInterface = fp->GetSettingsInterface();
            if (m_rayTracingDebugSettingsInterface)
            {
                OnConfigurationChanged();
            }
            fp->OnRayTracingDebugComponentAdded();
        }
        RayTracingDebugRequestBus::Handler::BusConnect(m_entityId);
    }

    void RayTracingDebugComponentController::Deactivate()
    {
        auto fp{ RPI::Scene::GetFeatureProcessorForEntity<RayTracingDebugFeatureProcessorInterface>(m_entityId) };
        if (fp)
        {
            fp->OnRayTracingDebugComponentRemoved();
        }

        RayTracingDebugRequestBus::Handler::BusDisconnect(m_entityId);
        m_rayTracingDebugSettingsInterface = nullptr;
        m_entityId.SetInvalid();
    }

    void RayTracingDebugComponentController::SetConfiguration(const RayTracingDebugComponentConfig& config)
    {
        m_configuration = config;
        OnConfigurationChanged();
    }

    const RayTracingDebugComponentConfig& RayTracingDebugComponentController::GetConfiguration() const
    {
        return m_configuration;
    }

    void RayTracingDebugComponentController::OnConfigurationChanged()
    {
        m_configuration.CopySettingsTo(m_rayTracingDebugSettingsInterface);
    }
} // namespace AZ::Render
