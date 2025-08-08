/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <PostProcess/AmbientOcclusion/AoComponentController.h>

namespace AZ
{
    namespace Render
    {
        void AoComponentController::Reflect(ReflectContext* context)
        {
            AoComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<AoComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &AoComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<AoRequestBus>("AoRequestBus")

                    // Auto-gen behavior context...
#define PARAM_EVENT_BUS AoRequestBus::Events
#include <Atom/Feature/ParamMacros/StartParamBehaviorContext.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef PARAM_EVENT_BUS

                    ;
            }
        }

        void AoComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AoService"));
        }

        void AoComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AoService"));
        }

        void AoComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        AoComponentController::AoComponentController(const AoComponentConfig& config)
            : m_configuration(config)
        {
        }

        void AoComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostProcessFeatureProcessorInterface* fp = RPI::Scene::GetFeatureProcessorForEntity<PostProcessFeatureProcessorInterface>(m_entityId);
            if (fp)
            {
                m_postProcessInterface = fp->GetOrCreateSettingsInterface(m_entityId);
                if (m_postProcessInterface)
                {
                    m_ssaoSettingsInterface = m_postProcessInterface->GetOrCreateAoSettingsInterface();
                    OnConfigChanged();
                }
            }
            AoRequestBus::Handler::BusConnect(m_entityId);
        }

        void AoComponentController::Deactivate()
        {
            AoRequestBus::Handler::BusDisconnect(m_entityId);

            if (m_postProcessInterface)
            {
                m_postProcessInterface->RemoveAoSettingsInterface();
            }

            m_postProcessInterface = nullptr;
            m_ssaoSettingsInterface = nullptr;
            m_entityId.SetInvalid();
        }

        // Getters & Setters...

        void AoComponentController::SetConfiguration(const AoComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const AoComponentConfig& AoComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void AoComponentController::OnConfigChanged()
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
        ValueType AoComponentController::Get##Name() const                                            \
        {                                                                                               \
            return m_configuration.MemberName;                                                          \
        }                                                                                               \
        void AoComponentController::Set##Name(ValueType val)                                          \
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
        OverrideValueType AoComponentController::Get##Name##Override() const                          \
        {                                                                                               \
            return m_configuration.MemberName##Override;                                                \
        }                                                                                               \
        void AoComponentController::Set##Name##Override(OverrideValueType val)                        \
        {                                                                                               \
            m_configuration.MemberName##Override = val;                                                 \
            if(m_ssaoSettingsInterface)                                                                 \
            {                                                                                           \
                m_ssaoSettingsInterface->Set##Name##Override(val);                                      \
                m_ssaoSettingsInterface->OnConfigChanged();                                             \
            }                                                                                           \
        }                                                                                               \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <Atom/Feature/ParamMacros/EndParams.inl>


    } // namespace Render
} // namespace AZ
