/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Asset/AssetSerializer.h>

#include <Atom/RPI.Public/Scene.h>

#include <ACES/Aces.h>

#include <PostProcess/LookModification/LookModificationComponentController.h>

namespace AZ
{
    namespace Render
    {
        void LookModificationComponentController::Reflect(ReflectContext* context)
        {
            LookModificationComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<LookModificationComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &LookModificationComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<LookModificationRequestBus>("LookModificationRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)

                    // Auto-gen behavior context...
#define PARAM_EVENT_BUS LookModificationRequestBus::Events
#include <Atom/Feature/ParamMacros/StartParamBehaviorContext.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef PARAM_EVENT_BUS

                    ;
            }
        }

        void LookModificationComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("LookModificationService"));
        }

        void LookModificationComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("LookModificationService"));
        }

        void LookModificationComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        LookModificationComponentController::LookModificationComponentController(const LookModificationComponentConfig& config)
            : m_configuration(config)
        {
        }

        void LookModificationComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostProcessFeatureProcessorInterface* fp = RPI::Scene::GetFeatureProcessorForEntity<PostProcessFeatureProcessorInterface>(m_entityId);
            if (fp)
            {
                m_postProcessInterface = fp->GetOrCreateSettingsInterface(m_entityId);
                if (m_postProcessInterface)
                {
                    m_settingsInterface = m_postProcessInterface->GetOrCreateLookModificationSettingsInterface();
                    OnConfigChanged();
                }
            }
            LookModificationRequestBus::Handler::BusConnect(m_entityId);
        }

        void LookModificationComponentController::Deactivate()
        {
            LookModificationRequestBus::Handler::BusDisconnect(m_entityId);

            if (m_postProcessInterface)
            {
                m_postProcessInterface->RemoveLookModificationSettingsInterface();
            }

            m_postProcessInterface = nullptr;
            m_settingsInterface = nullptr;
            m_entityId.SetInvalid();
        }

        // Getters & Setters...

        void LookModificationComponentController::SetConfiguration(const LookModificationComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const LookModificationComponentConfig& LookModificationComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void LookModificationComponentController::OnConfigChanged()
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
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType LookModificationComponentController::Get##Name() const                                \
        {                                                                                               \
            return m_configuration.MemberName;                                                          \
        }                                                                                               \
        void LookModificationComponentController::Set##Name(ValueType val)                              \
        {                                                                                               \
            if(m_settingsInterface)                                                                     \
            {                                                                                           \
                m_settingsInterface->Set##Name(val);                                                    \
                m_settingsInterface->OnConfigChanged();                                                 \
                m_configuration.MemberName = m_settingsInterface->Get##Name();                          \
            }                                                                                           \
            else                                                                                        \
            {                                                                                           \
                m_configuration.MemberName = val;                                                       \
            }                                                                                           \
        }                                                                                               \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        OverrideValueType LookModificationComponentController::Get##Name##Override() const              \
        {                                                                                               \
            return m_configuration.MemberName##Override;                                                \
        }                                                                                               \
        void LookModificationComponentController::Set##Name##Override(OverrideValueType val)            \
        {                                                                                               \
            m_configuration.MemberName##Override = val;                                                 \
            if(m_settingsInterface)                                                                     \
            {                                                                                           \
                m_settingsInterface->Set##Name##Override(val);                                          \
                m_settingsInterface->OnConfigChanged();                                                 \
            }                                                                                           \
        }
#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>


    } // namespace Render
} // namespace AZ
