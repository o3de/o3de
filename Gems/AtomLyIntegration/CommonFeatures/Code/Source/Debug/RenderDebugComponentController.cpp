/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/RPI.Public/Scene.h>
#include <Debug/RenderDebugComponentController.h>

namespace AZ::Render
{
    void RenderDebugComponentController::Reflect(ReflectContext* context)
    {
        RenderDebugComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<RenderDebugComponentController>()
                ->Version(0)
                ->Field("Configuration", &RenderDebugComponentController::m_configuration);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RenderDebugRequestBus>("RenderDebugRequestBus")

                // Auto-gen behavior context...
#define PARAM_EVENT_BUS RenderDebugRequestBus::Events
#include <Atom/Feature/ParamMacros/StartParamBehaviorContext.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef PARAM_EVENT_BUS

                ;
        }
    }

    void RenderDebugComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RenderDebugService"));
    }

    void RenderDebugComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RenderDebugService"));
    }

    void RenderDebugComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required)
    }

    RenderDebugComponentController::RenderDebugComponentController(const RenderDebugComponentConfig& config)
        : m_configuration(config)
    {
    }

    void RenderDebugComponentController::Activate(EntityId entityId)
    {
        m_entityId = entityId;

        RenderDebugFeatureProcessorInterface* fp = RPI::Scene::GetFeatureProcessorForEntity<RenderDebugFeatureProcessorInterface>(m_entityId);
        if (fp)
        {
            m_renderDebugSettingsInterface = fp->GetSettingsInterface();
            if (m_renderDebugSettingsInterface)
            {
                m_configuration.CopySettingsFrom(m_renderDebugSettingsInterface);
            }
            fp->OnRenderDebugComponentAdded();
        }
        RenderDebugRequestBus::Handler::BusConnect(m_entityId);
    }

    void RenderDebugComponentController::Deactivate()
    {
        RenderDebugFeatureProcessorInterface* fp = RPI::Scene::GetFeatureProcessorForEntity<RenderDebugFeatureProcessorInterface>(m_entityId);
        if (fp)
        {
            fp->OnRenderDebugComponentRemoved();
        }

        RenderDebugRequestBus::Handler::BusDisconnect(m_entityId);
        m_renderDebugSettingsInterface = nullptr;
        m_entityId.SetInvalid();
    }

    // Getters & Setters...

    void RenderDebugComponentController::SetConfiguration(const RenderDebugComponentConfig& config)
    {
        m_configuration = config;
        OnConfigChanged();
    }

    const RenderDebugComponentConfig& RenderDebugComponentController::GetConfiguration() const
    {
        return m_configuration;
    }

    void RenderDebugComponentController::OnConfigChanged()
    {
        if (m_renderDebugSettingsInterface)
        {
            m_configuration.CopySettingsTo(m_renderDebugSettingsInterface);
        }
    }

    // Auto-gen getter/setter function definitions...
    // The setter functions will set the values on the Atom settings class, then get the value back
    // from the settings class to set the local configuration. This is in case the settings class
    // applies some custom logic that results in the set value being different from the input
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                              \
    ValueType RenderDebugComponentController::Get##Name() const                                     \
    {                                                                                               \
        return m_configuration.MemberName;                                                          \
    }                                                                                               \
    void RenderDebugComponentController::Set##Name(ValueType val)                                   \
    {                                                                                               \
        if(m_renderDebugSettingsInterface)                                                          \
        {                                                                                           \
            m_renderDebugSettingsInterface->Set##Name(val);                                         \
            m_configuration.CopySettingsFrom(m_renderDebugSettingsInterface);                       \
        }                                                                                           \
        else                                                                                        \
        {                                                                                           \
            m_configuration.MemberName = val;                                                       \
        }                                                                                           \
    }                                                                                               \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                      \
    OverrideValueType RenderDebugComponentController::Get##Name##Override() const                   \
    {                                                                                               \
        return m_configuration.MemberName##Override;                                                \
    }                                                                                               \
    void RenderDebugComponentController::Set##Name##Override(OverrideValueType val)                 \
    {                                                                                               \
        m_configuration.MemberName##Override = val;                                                 \
        if(m_renderDebugSettingsInterface)                                                          \
        {                                                                                           \
            m_renderDebugSettingsInterface->Set##Name##Override(val);                               \
            m_configuration.CopySettingsFrom(m_renderDebugSettingsInterface);                       \
        }                                                                                           \
    }                                                                                               \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

}
