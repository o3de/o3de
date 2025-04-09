/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <ScreenSpace/DeferredFogComponentController.h>

namespace AZ
{
    namespace Render
    {
        void DeferredFogComponentController::Reflect(ReflectContext* context)
        {
            DeferredFogComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DeferredFogComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &DeferredFogComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<DeferredFogRequestsBus>("DeferredFogRequestsBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)

                    // Auto-gen behavior context...
#define PARAM_EVENT_BUS DeferredFogRequestsBus::Events
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ->Event("Set" #Name, &PARAM_EVENT_BUS::Set##Name)                                               \
        ->Event("Get" #Name, &PARAM_EVENT_BUS::Get##Name)                                               \
        ->VirtualProperty(#Name, "Get" #Name, "Set" #Name)                                              \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef PARAM_EVENT_BUS

                    ;
            }
        }

        void DeferredFogComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("DeferredFogService"));
        }

        void DeferredFogComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("DeferredFogService"));
        }

        void DeferredFogComponentController::GetRequiredServices( [[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {   // In the future deferred fog might be required to be anchored to activation locations
            required.push_back(AZ_CRC_CE("PostFXLayerService"));   // For aggregated settings, otherwise settings are not updated
        }

        DeferredFogComponentController::DeferredFogComponentController(const DeferredFogComponentConfig& config)
            : m_configuration(config)
        {
        }

        void DeferredFogComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostProcessFeatureProcessorInterface* featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<PostProcessFeatureProcessorInterface>(m_entityId);
            if (featureProcessor)
            {
                m_postProcessInterface = featureProcessor->GetOrCreateSettingsInterface(m_entityId);
                if (m_postProcessInterface)
                {
                    m_settingsInterface = m_postProcessInterface->GetOrCreateDeferredFogSettingsInterface();
                    OnConfigChanged();
                }
            }
            DeferredFogRequestsBus::Handler::BusConnect(m_entityId);
        }

        void DeferredFogComponentController::Deactivate()
        {
            DeferredFogRequestsBus::Handler::BusDisconnect(m_entityId);

            if (m_postProcessInterface)
            {
                // turn off the lights before leaving
                m_settingsInterface->SetEnabled(false);     
                m_settingsInterface->OnSettingsChanged();
                // Now you can leave
                m_postProcessInterface->RemoveDeferredFogSettingsInterface();
            }
            m_settingsInterface = nullptr;
            m_entityId.SetInvalid();
        }

        void DeferredFogComponentController::SetConfiguration(const DeferredFogComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const DeferredFogComponentConfig& DeferredFogComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void DeferredFogComponentController::OnConfigChanged()
        {
            if (m_settingsInterface)
            {
                // Set SRG constants
                m_configuration.CopySettingsTo(m_settingsInterface);
                m_settingsInterface->OnSettingsChanged();
            }
        }

        // Auto generated getter/setter functions
        // The setter functions will set the values on the Atom settings class, then get the value back
        // from the settings class to set the local configuration. This is in case the settings class
        // applies some custom logic that results in the set value being different from the input
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType DeferredFogComponentController::Get##Name() const                                     \
        {                                                                                               \
            return m_configuration.Get##Name();                                                         \
        }                                                                                               \
        void DeferredFogComponentController::Set##Name(ValueType val)                                   \
        {                                                                                               \
            if(m_settingsInterface)                                                                     \
            {                                                                                           \
                m_configuration.Set##Name( val );                                                       \
                m_settingsInterface->Set##Name(val);                                                    \
                m_settingsInterface->OnSettingsChanged();                                               \
            }                                                                                           \
            else                                                                                        \
            {                                                                                           \
                m_configuration.Set##Name( val );                                                       \
            }                                                                                           \
        }                                                                                               \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

    } // namespace Render
} // namespace AZ
