/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomLyIntegration/CommonFeatures/Debug/RenderDebugBus.h>
#include <AtomLyIntegration/CommonFeatures/Debug/RenderDebugComponentConfig.h>

#include <Atom/Feature/Debug/RenderDebugSettingsInterface.h>
#include <Atom/Feature/Debug/RenderDebugSettingsInterface.h>
#include <Atom/Feature/Debug/RenderDebugFeatureProcessorInterface.h>

namespace AZ::Render
{
    class RenderDebugComponentController final
        : public RenderDebugRequestBus::Handler
    {
    public:
        friend class RenderDebugEditorComponent;

        AZ_TYPE_INFO(AZ::Render::RenderDebugComponentController, "{365E4B90-7145-4803-B990-B6D3E0C4B80B}");
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        RenderDebugComponentController() = default;
        RenderDebugComponentController(const RenderDebugComponentConfig& config);

        void Activate(EntityId entityId);
        void Deactivate();
        void SetConfiguration(const RenderDebugComponentConfig& config);
        const RenderDebugComponentConfig& GetConfiguration() const;

        // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

    private:
        AZ_DISABLE_COPY(RenderDebugComponentController);

        void OnConfigChanged();

        RenderDebugSettingsInterface* m_renderDebugSettingsInterface = nullptr;
        RenderDebugComponentConfig m_configuration;
        EntityId m_entityId;
    };

}
