/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <PostProcess/Ssao/SsaoComponentController.h>

namespace AZ
{
    namespace Render
    {
        void SsaoComponentController::Reflect(ReflectContext* context)
        {
            SsaoComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SsaoComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &SsaoComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<SsaoRequestBus>("SsaoRequestBus")

                    // Auto-gen behavior context...
#define PARAM_EVENT_BUS SsaoRequestBus::Events
#include <Atom/Feature/ParamMacros/StartParamBehaviorContext.inl>
#include <Atom/Feature/PostProcess/Ssao/SsaoParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef PARAM_EVENT_BUS

                    ;
            }
        }

        void SsaoComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SsaoService"));
        }

        void SsaoComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SsaoService"));
        }

        void SsaoComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        SsaoComponentController::SsaoComponentController(const SsaoComponentConfig& config)
            : m_configuration(config)
        {
        }

        void SsaoComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostProcessFeatureProcessorInterface* fp = RPI::Scene::GetFeatureProcessorForEntity<PostProcessFeatureProcessorInterface>(m_entityId);
            if (fp)
            {
                m_postProcessInterface = fp->GetOrCreateSettingsInterface(m_entityId);
                if (m_postProcessInterface)
                {
                    m_ssaoSettingsInterface = m_postProcessInterface->GetOrCreateSsaoSettingsInterface();
                    OnConfigChanged();
                }
            }
            SsaoRequestBus::Handler::BusConnect(m_entityId);
        }

        void SsaoComponentController::Deactivate()
        {
            SsaoRequestBus::Handler::BusDisconnect(m_entityId);

            if (m_postProcessInterface)
            {
                m_postProcessInterface->RemoveSsaoSettingsInterface();
            }

            m_postProcessInterface = nullptr;
            m_ssaoSettingsInterface = nullptr;
            m_entityId.SetInvalid();
        }

        // Getters & Setters...

        void SsaoComponentController::SetConfiguration(const SsaoComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const SsaoComponentConfig& SsaoComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void SsaoComponentController::OnConfigChanged()
        {
            if (m_ssaoSettingsInterface)
            {
                m_configuration.CopySettingsTo(m_ssaoSettingsInterface);
                m_ssaoSettingsInterface->OnConfigChanged();
            }
        }

        // Auto-gen getter/setter function definitions...
        // The setter functions will set the values on the Atom settings class, then get the value back
        // from the settings class to set the local configuration. This is in case the settings class
        // applies some custom logic that results in the set value being different from the input
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType SsaoComponentController::Get##Name() const                                            \
        {                                                                                               \
            return m_configuration.MemberName;                                                          \
        }                                                                                               \
        void SsaoComponentController::Set##Name(ValueType val)                                          \
        {                                                                                               \
            if(m_ssaoSettingsInterface)                                                                 \
            {                                                                                           \
                m_ssaoSettingsInterface->Set##Name(val);                                                \
                m_ssaoSettingsInterface->OnConfigChanged();                                             \
                m_configuration.MemberName = m_ssaoSettingsInterface->Get##Name();                      \
            }                                                                                           \
            else                                                                                        \
            {                                                                                           \
                m_configuration.MemberName = val;                                                       \
            }                                                                                           \
        }                                                                                               \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        OverrideValueType SsaoComponentController::Get##Name##Override() const                          \
        {                                                                                               \
            return m_configuration.MemberName##Override;                                                \
        }                                                                                               \
        void SsaoComponentController::Set##Name##Override(OverrideValueType val)                        \
        {                                                                                               \
            m_configuration.MemberName##Override = val;                                                 \
            if(m_ssaoSettingsInterface)                                                                 \
            {                                                                                           \
                m_ssaoSettingsInterface->Set##Name##Override(val);                                      \
                m_ssaoSettingsInterface->OnConfigChanged();                                             \
            }                                                                                           \
        }                                                                                               \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/Ssao/SsaoParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>


    } // namespace Render
} // namespace AZ
