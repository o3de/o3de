/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/ChromaticAberration/ChromaticAberrationComponentController.h>

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        void ChromaticAberrationComponentController::Reflect(ReflectContext* context)
        {
            ChromaticAberrationComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ChromaticAberrationComponentController>()
                ->Version(0)
                ->Field("Configuration", &ChromaticAberrationComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<ChromaticAberrationRequestBus>("ChromaticAberrationRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                // Auto-gen behavior context...
#define PARAM_EVENT_BUS ChromaticAberrationRequestBus::Events
#include <Atom/Feature/ParamMacros/StartParamBehaviorContext.inl>
#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef PARAM_EVENT_BUS

                    ;
            }
        }

        void ChromaticAberrationComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ChromaticAberrationService"));
        }

        void ChromaticAberrationComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ChromaticAberrationService"));
        }

        void ChromaticAberrationComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        ChromaticAberrationComponentController::ChromaticAberrationComponentController(const ChromaticAberrationComponentConfig& config)
            : m_configuration(config)
        {
        }

        void ChromaticAberrationComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostProcessFeatureProcessorInterface* fp =
                RPI::Scene::GetFeatureProcessorForEntity<PostProcessFeatureProcessorInterface>(m_entityId);
            if (fp)
            {
                m_postProcessInterface = fp->GetOrCreateSettingsInterface(m_entityId);
                if (m_postProcessInterface)
                {
                    m_settingsInterface = m_postProcessInterface->GetOrCreateChromaticAberrationSettingsInterface();
                    OnConfigChanged();
                }
            }
            ChromaticAberrationRequestBus::Handler::BusConnect(m_entityId);
        }

        void ChromaticAberrationComponentController::Deactivate()
        {
            ChromaticAberrationRequestBus::Handler::BusDisconnect(m_entityId);

            if (m_postProcessInterface)
            {
                m_postProcessInterface->RemoveChromaticAberrationSettingsInterface();
            }

            m_postProcessInterface = nullptr;
            m_settingsInterface = nullptr;
            m_entityId.SetInvalid();
        }

        // Getters & Setters...

        void ChromaticAberrationComponentController::SetConfiguration(const ChromaticAberrationComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const ChromaticAberrationComponentConfig& ChromaticAberrationComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void ChromaticAberrationComponentController::OnConfigChanged()
        {
            if (m_settingsInterface)
            {
                m_configuration.CopySettingsTo(m_settingsInterface);
                m_settingsInterface->OnConfigChanged();
            }
        }

        // Auto-gen getter/setter function definitions...
        // The setter functions will set the values on the Atom settings class, then get the value back
        // from the settings class to set the local configuration. This is in case the settings class
        // applies some custom logic that results in the set value being different from the input
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                                                     \
    ValueType ChromaticAberrationComponentController::Get##Name() const                                                                              \
    {                                                                                                                                      \
        return m_configuration.MemberName;                                                                                                 \
    }                                                                                                                                      \
    void ChromaticAberrationComponentController::Set##Name(ValueType val)                                                                            \
    {                                                                                                                                      \
        if (m_settingsInterface)                                                                                                           \
        {                                                                                                                                  \
            m_settingsInterface->Set##Name(val);                                                                                           \
            m_settingsInterface->OnConfigChanged();                                                                                        \
            m_configuration.MemberName = m_settingsInterface->Get##Name();                                                                 \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            m_configuration.MemberName = val;                                                                                              \
        }                                                                                                                                  \
    }

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                                                             \
    OverrideValueType ChromaticAberrationComponentController::Get##Name##Override() const                                                            \
    {                                                                                                                                      \
        return m_configuration.MemberName##Override;                                                                                       \
    }                                                                                                                                      \
    void ChromaticAberrationComponentController::Set##Name##Override(OverrideValueType val)                                                          \
    {                                                                                                                                      \
        m_configuration.MemberName##Override = val;                                                                                        \
        if (m_settingsInterface)                                                                                                           \
        {                                                                                                                                  \
            m_settingsInterface->Set##Name##Override(val);                                                                                 \
            m_settingsInterface->OnConfigChanged();                                                                                        \
        }                                                                                                                                  \
    }

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
    } // namespace Render
} // namespace AZ
