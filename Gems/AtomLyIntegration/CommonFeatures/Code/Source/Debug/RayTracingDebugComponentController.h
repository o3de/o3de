/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomLyIntegration/CommonFeatures/Debug/RayTracingDebugBus.h>
#include <AtomLyIntegration/CommonFeatures/Debug/RayTracingDebugComponentConfig.h>

namespace AZ::Render
{
    class RayTracingDebugComponentController final : public RayTracingDebugRequestBus::Handler
    {
        friend class RayTracingDebugEditorComponent;

    public:
        AZ_TYPE_INFO(RayTracingDebugComponentController, "{7B1CAB96-6B9E-46C4-BBDE-B140E8082CEB}");
        static void Reflect(ReflectContext* context);

        RayTracingDebugComponentController() = default;
        RayTracingDebugComponentController(const RayTracingDebugComponentConfig& config);

        void Activate(EntityId entityId);
        void Deactivate();
        void SetConfiguration(const RayTracingDebugComponentConfig& config);
        const RayTracingDebugComponentConfig& GetConfiguration() const;

        // clang-format off
        // Generate function override declarations (functions definitions in .cpp)
        #include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
        #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
        #include <Atom/Feature/ParamMacros/EndParams.inl>
        // clang-format on

    private:
        void OnConfigurationChanged();

        RayTracingDebugSettingsInterface* m_rayTracingDebugSettingsInterface{ nullptr };
        EntityId m_entityId{ EntityId::InvalidEntityId };
        RayTracingDebugComponentConfig m_configuration;
    };
} // namespace AZ::Render
