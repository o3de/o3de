/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <PostProcess/DepthOfField/DepthOfFieldComponentController.h>

namespace AZ
{
    namespace Render
    {
        void DepthOfFieldComponentController::Reflect(ReflectContext* context)
        {
            DepthOfFieldComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DepthOfFieldComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &DepthOfFieldComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<DepthOfFieldRequestBus>("DepthOfFieldRequestBus")

                    // Auto-gen behavior context...
#define PARAM_EVENT_BUS DepthOfFieldRequestBus::Events
#include <Atom/Feature/ParamMacros/StartParamBehaviorContext.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef PARAM_EVENT_BUS

                    ;
            }
        }

        void DepthOfFieldComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("DepthOfFieldService"));
        }

        void DepthOfFieldComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("DepthOfFieldService"));
        }

        void DepthOfFieldComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        DepthOfFieldComponentController::DepthOfFieldComponentController(const DepthOfFieldComponentConfig& config)
            : m_configuration(config)
        {
        }

        void DepthOfFieldComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostProcessFeatureProcessorInterface* fp = RPI::Scene::GetFeatureProcessorForEntity<PostProcessFeatureProcessorInterface>(m_entityId);
            if (fp)
            {
                m_postProcessInterface = fp->GetOrCreateSettingsInterface(m_entityId);
                if (m_postProcessInterface)
                {
                    m_depthOfFieldSettingsInterface = m_postProcessInterface->GetOrCreateDepthOfFieldSettingsInterface();
                    OnConfigChanged();
                }
            }
            DepthOfFieldRequestBus::Handler::BusConnect(m_entityId);
        }

        void DepthOfFieldComponentController::Deactivate()
        {
            DepthOfFieldRequestBus::Handler::BusDisconnect(m_entityId);

            if (m_postProcessInterface)
            {
                m_postProcessInterface->RemoveDepthOfFieldSettingsInterface();
            }

            m_postProcessInterface = nullptr;
            m_depthOfFieldSettingsInterface = nullptr;
            m_entityId.SetInvalid();
        }

        // Getters & Setters...

        void DepthOfFieldComponentController::SetConfiguration(const DepthOfFieldComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const DepthOfFieldComponentConfig& DepthOfFieldComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void DepthOfFieldComponentController::OnConfigChanged()
        {
            if (m_depthOfFieldSettingsInterface)
            {
                m_configuration.CopySettingsTo(m_depthOfFieldSettingsInterface);
                m_depthOfFieldSettingsInterface->OnConfigChanged();
                UpdateInferredParams();
            }
        }

        void DepthOfFieldComponentController::UpdateInferredParams()
        {
            m_configuration.m_fNumber = m_depthOfFieldSettingsInterface->GetFNumber();
        }

        // Auto-gen getter/setter function definitions...
        // The setter functions will set the values on the Atom settings class, then get the value back
        // from the settings class to set the local configuration. This is in case the settings class
        // applies some custom logic that results in the set value being different from the input
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType DepthOfFieldComponentController::Get##Name() const                                    \
        {                                                                                               \
            return m_configuration.MemberName;                                                          \
        }                                                                                               \
        void DepthOfFieldComponentController::Set##Name(ValueType val)                                  \
        {                                                                                               \
            if(m_depthOfFieldSettingsInterface)                                                         \
            {                                                                                           \
                m_depthOfFieldSettingsInterface->Set##Name(val);                                        \
                m_depthOfFieldSettingsInterface->OnConfigChanged();                                     \
                m_configuration.MemberName = m_depthOfFieldSettingsInterface->Get##Name();              \
                UpdateInferredParams();                                                                 \
            }                                                                                           \
            else                                                                                        \
            {                                                                                           \
                m_configuration.MemberName = val;                                                       \
            }                                                                                           \
        }                                                                                               \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        OverrideValueType DepthOfFieldComponentController::Get##Name##Override() const                  \
        {                                                                                               \
            return m_configuration.MemberName##Override;                                                \
        }                                                                                               \
        void DepthOfFieldComponentController::Set##Name##Override(OverrideValueType val)                \
        {                                                                                               \
            m_configuration.MemberName##Override = val;                                                 \
            if(m_depthOfFieldSettingsInterface)                                                         \
            {                                                                                           \
                m_depthOfFieldSettingsInterface->Set##Name##Override(val);                              \
                m_depthOfFieldSettingsInterface->OnConfigChanged();                                     \
            }                                                                                           \
        }                                                                                               \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>


    } // namespace Render
} // namespace AZ
