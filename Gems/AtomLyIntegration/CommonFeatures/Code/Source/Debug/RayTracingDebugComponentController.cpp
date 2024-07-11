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

        if (auto* serializeContext{ azrtti_cast<SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugComponentController>()
                ->Version(0)
                ->Field("Configuration", &RayTracingDebugComponentController::m_configuration)
            ;
            // clang-format on
        }

        if (auto* behaviorContext{ azrtti_cast<BehaviorContext*>(context) })
        {
            // clang-format off
            behaviorContext->EBus<RayTracingDebugRequestBus>("RayTracingDebugRequestBus")
                // Generate behavior context
                #define PARAM_EVENT_BUS RayTracingDebugRequestBus::Events
                #include <Atom/Feature/ParamMacros/StartParamBehaviorContext.inl>
                #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
                #include <Atom/Feature/ParamMacros/EndParams.inl>
                #undef PARAM_EVENT_BUS
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

    // clang-format off
    // Generate getter/setter function definitions.
    // The setter functions will set the values on the Atom settings class, then get the value back
    // from the settings class to set the local configuration. This is in case the settings class
    // applies some custom logic that results in the set value being different from the input
    #define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)              \
    ValueType RayTracingDebugComponentController::Get##Name() const                     \
    {                                                                                   \
        return m_configuration.MemberName;                                              \
    }                                                                                   \
    void RayTracingDebugComponentController::Set##Name(ValueType val)                   \
    {                                                                                   \
        if (m_rayTracingDebugSettingsInterface)                                         \
        {                                                                               \
            m_rayTracingDebugSettingsInterface->Set##Name(val);                         \
            m_configuration.CopySettingsFrom(m_rayTracingDebugSettingsInterface);       \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            m_configuration.MemberName = val;                                           \
        }                                                                               \
    }                                                                                   \

    #define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)      \
    OverrideValueType RayTracingDebugComponentController::Get##Name##Override() const   \
    {                                                                                   \
        return m_configuration.MemberName##Override;                                    \
    }                                                                                   \
    void RayTracingDebugComponentController::Set##Name##Override(OverrideValueType val) \
    {                                                                                   \
        m_configuration.MemberName##Override = val;                                     \
        if (m_RayTracingDebugSettingsInterface)                                         \
        {                                                                               \
            m_rayTracingDebugSettingsInterface->Set##Name##Override(val);               \
            m_configuration.CopySettingsFrom(m_rayTracingDebugSettingsInterface);       \
        }                                                                               \
    }                                                                                   \

    #include <Atom/Feature/ParamMacros/MapAllCommon.inl>
    #include <Atom/Feature/Debug/RayTracingDebugParams.inl>
    #include <Atom/Feature/ParamMacros/EndParams.inl>
    // clang-format on
} // namespace AZ::Render
