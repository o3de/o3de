/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <PostProcess/ColorGrading/HDRColorGradingComponentController.h>

namespace AZ
{
    namespace Render
    {
        void HDRColorGradingComponentController::Reflect(ReflectContext* context)
        {
            HDRColorGradingComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<HDRColorGradingComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &HDRColorGradingComponentController::m_configuration);
            }

            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<HDRColorGradingRequestBus>("HDRColorGradingRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)

                    // Auto-gen behavior context...
#define PARAM_EVENT_BUS HDRColorGradingRequestBus::Events
#include <Atom/Feature/ParamMacros/StartParamBehaviorContext.inl>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef PARAM_EVENT_BUS

                    ;
            }
        }

        void HDRColorGradingComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("HDRColorGradingService"));
        }

        void HDRColorGradingComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("HDRColorGradingService"));
            incompatible.push_back(AZ_CRC_CE("LookModificationService"));
        }

        void HDRColorGradingComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        HDRColorGradingComponentController::HDRColorGradingComponentController(const HDRColorGradingComponentConfig& config)
            : m_configuration(config)
        {
        }

        void HDRColorGradingComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostProcessFeatureProcessorInterface* fp =
                RPI::Scene::GetFeatureProcessorForEntity<PostProcessFeatureProcessorInterface>(m_entityId);
            if (fp)
            {
                m_postProcessInterface = fp->GetOrCreateSettingsInterface(m_entityId);
                if (m_postProcessInterface)
                {
                    m_settingsInterface = m_postProcessInterface->GetOrCreateHDRColorGradingSettingsInterface();
                    OnConfigChanged();
                }
            }
            HDRColorGradingRequestBus::Handler::BusConnect(m_entityId);
        }

        void HDRColorGradingComponentController::Deactivate()
        {
            HDRColorGradingRequestBus::Handler::BusDisconnect(m_entityId);

            if (m_postProcessInterface)
            {
                m_postProcessInterface->RemoveHDRColorGradingSettingsInterface();
            }

            m_postProcessInterface = nullptr;
            m_settingsInterface = nullptr;
            m_entityId.SetInvalid();
        }

        void HDRColorGradingComponentController::SetConfiguration(const HDRColorGradingComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const HDRColorGradingComponentConfig& HDRColorGradingComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void HDRColorGradingComponentController::OnConfigChanged()
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
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                                                         \
        ValueType HDRColorGradingComponentController::Get##Name() const                                                                        \
        {                                                                                                                                      \
            return m_configuration.MemberName;                                                                                                 \
        }                                                                                                                                      \
        void HDRColorGradingComponentController::Set##Name(ValueType val)                                                                      \
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

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
    } // namespace Render
} // namespace AZ
