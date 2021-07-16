/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/PlatformDef.h>

#include "EMotionFX_precompiled.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <Integration/Components/SimpleLODComponent.h>
#include <MCore/Source/AttributeString.h>

#include <MathConversion.h>
#include <IRenderAuxGeom.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        void SimpleLODComponent::Configuration::Reflect(AZ::ReflectContext *context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<Configuration>()
                    ->Version(2)
                    ->Field("LODDistances", &Configuration::m_lodDistances)
                    ->Field("EnableLODSampling", &Configuration::m_enableLodSampling)
                    ->Field("LODSampleRates", &Configuration::m_lodSampleRates)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SimpleLODComponent::Configuration>("Configuration", "The LOD Configuration.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(0, &SimpleLODComponent::Configuration::m_lodDistances,
                            "LOD distance (Max)", "The maximum camera distance of this LOD.")
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->ElementAttribute(AZ::Edit::Attributes::Step, 0.01f)
                            ->ElementAttribute(AZ::Edit::Attributes::Suffix, " m")
                        ->DataElement(0, &SimpleLODComponent::Configuration::m_enableLodSampling,
                            "Enable LOD anim graph sampling", "AnimGraph sample rate will adjust based on LOD level.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->DataElement(0, &SimpleLODComponent::Configuration::m_lodSampleRates,
                            "Anim graph sample rates", "The sample rate of anim graph based on LOD. Setting it to O means the maximum sample rate.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &SimpleLODComponent::Configuration::GetEnableLodSampling)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->ElementAttribute(AZ::Edit::Attributes::Step, 1.0f);
                }
            }
        }

        void SimpleLODComponent::Configuration::Reset()
        {
            m_lodDistances.clear();
        }

        void SimpleLODComponent::Configuration::GenerateDefaultValue(AZ::u32 numLODs)
        {
            if (numLODs != m_lodDistances.size())
            {
                // Generate the default LOD (max) distance to 10, 20, 30....
                m_lodDistances.resize(numLODs);
                for (AZ::u32 i = 0; i < numLODs; ++i)
                {
                    m_lodDistances[i] = i * 10.0f + 10.0f;
                }
            }

            if (numLODs != m_lodSampleRates.size())
            {
                // Generate the default LOD Sample Rate to 140, 60, 45, 25, 15, 10
                const float defaultSampleRate[] = {140.0f, 60.0f, 45.0f, 25.0f, 15.0f, 10.0f};
                m_lodSampleRates.resize(numLODs);
                for (AZ::u32 i = 0; i < numLODs; ++i)
                {
                    m_lodSampleRates[i] = defaultSampleRate[i];
                }
            }
        }

        bool SimpleLODComponent::Configuration::GetEnableLodSampling()
        {
            return m_enableLodSampling;
        }

        void SimpleLODComponent::Reflect(AZ::ReflectContext* context)
        {
            Configuration::Reflect(context);

            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SimpleLODComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Configuration", &SimpleLODComponent::m_configuration)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SimpleLODComponent>(
                        "Simple LOD distance", "The Simple LOD distance component alters the actor LOD level based on distance to camera")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }

        }

        SimpleLODComponent::SimpleLODComponent(const Configuration* config)
            : m_actorInstance(nullptr)
        {
            if (config)
            {
                m_configuration = *config;
            }
        }

        SimpleLODComponent::~SimpleLODComponent()
        {
        }

        void SimpleLODComponent::Init()
        {
        }

        void SimpleLODComponent::Activate()
        {
            ActorComponentNotificationBus::Handler::BusConnect(GetEntityId());
            AZ::TickBus::Handler::BusConnect();
        }

        void SimpleLODComponent::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();
        }

        void SimpleLODComponent::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
        {
            m_actorInstance = actorInstance;
        }

        void SimpleLODComponent::OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance)
        {
            AZ_UNUSED(actorInstance);
            m_actorInstance = nullptr;
        }

        void SimpleLODComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            AZ_UNUSED(deltaTime);
            AZ_UNUSED(time);
            UpdateLodLevelByDistance(m_actorInstance, m_configuration, GetEntityId());
        }

        AZ::u32 SimpleLODComponent::GetLodByDistance(const AZStd::vector<float>& distances, float distance)
        {
            const size_t max = distances.size();
            for (size_t i = 0; i < max; ++i)
            {
                const float rDistance = distances[i];
                if (distance < rDistance)
                {
                    return static_cast<AZ::u32>(i);
                }
            }

            return static_cast<AZ::u32>(max - 1);
        }

        void SimpleLODComponent::UpdateLodLevelByDistance(EMotionFX::ActorInstance * actorInstance, const Configuration& configuration, AZ::EntityId entityId)
        {
            if (actorInstance)
            {
                // Compute the distance between the camera and the entity
                AZ::Transform worldTransform;
                AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);
                const AZ::Vector3& worldPos = worldTransform.GetTranslation();

                auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
                if (!viewportContextManager)
                {
                    return;
                }

                AZ::RPI::ViewportContextPtr defaultViewportContext =
                    viewportContextManager->GetViewportContextByName(viewportContextManager->GetDefaultViewportContextName());
                const float distance = worldPos.GetDistance(defaultViewportContext->GetCameraTransform().GetTranslation());
                const AZ::u32 lodByDistance = GetLodByDistance(configuration.m_lodDistances, distance);
                actorInstance->SetLODLevel(lodByDistance);

                if (configuration.m_enableLodSampling)
                {
                    const float animGraphSampleRate = configuration.m_lodSampleRates[lodByDistance];
                    const float updateRateInSeconds = animGraphSampleRate > 0.0f ? 1.0f / animGraphSampleRate : 0.0f;
                    actorInstance->SetMotionSamplingRate(updateRateInSeconds);
                }
            }
        }
    } // namespace integration
} // namespace EMotionFX
