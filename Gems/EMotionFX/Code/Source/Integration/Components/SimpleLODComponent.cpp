/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/PlatformDef.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <Integration/Components/SimpleLODComponent.h>
#include <MCore/Source/AttributeString.h>
#include <EMotionFX/Source/ActorInstance.h>

#include <MathConversion.h>

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
                            ->ElementAttribute(AZ::Edit::Attributes::Min, 0.00f)
                        ->DataElement(0, &SimpleLODComponent::Configuration::m_enableLodSampling,
                            "Enable LOD anim graph sampling", "AnimGraph sample rate will adjust based on LOD level.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->DataElement(0, &SimpleLODComponent::Configuration::m_lodSampleRates,
                            "Anim graph sample rates", "The sample rate of anim graph based on LOD. Setting it to O means the maximum sample rate.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &SimpleLODComponent::Configuration::GetEnableLodSampling)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->ElementAttribute(AZ::Edit::Attributes::Step, 1.0f)
                            ->ElementAttribute(AZ::Edit::Attributes::Min, 0.0f);
                }
            }
        }

        void SimpleLODComponent::Configuration::Reset()
        {
            m_lodDistances.clear();
        }

        void SimpleLODComponent::Configuration::GenerateDefaultValue(size_t numLODs)
        {
            if (numLODs != m_lodDistances.size())
            {
                // Generate the default LOD (max) distance to 10, 20, 30....
                m_lodDistances.resize(numLODs);
                for (size_t i = 0; i < numLODs; ++i)
                {
                    m_lodDistances[i] = i * 10.0f + 10.0f;
                }
            }

            if (numLODs != m_lodSampleRates.size())
            {
                // Generate the default LOD Sample Rate to 140, 60, 45, 25, 15, 10, 10, 10, ...
                constexpr AZStd::array defaultSampleRate {140.0f, 60.0f, 45.0f, 25.0f, 15.0f, 10.0f};
                m_lodSampleRates.resize(numLODs, 10.0f);

                // Do not copy more than what fits in defaultSampleRates or numLODs.
                size_t copyCount = std::min(defaultSampleRate.size(), numLODs);
                AZStd::copy(begin(defaultSampleRate), begin(defaultSampleRate) + copyCount, begin(m_lodSampleRates));
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
            AZ::ApplicationTypeQuery appType;
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
            if (appType.IsHeadless())
            {
                return;
            }

            ActorComponentNotificationBus::Handler::BusConnect(GetEntityId());
            AZ::TickBus::Handler::BusConnect();

            // Remember the lod type and level so that we can set it back to the previous one on deactivation of the component.
            AZ::Render::MeshComponentRequestBus::EventResult(m_previousLodType,
                GetEntityId(),
                &AZ::Render::MeshComponentRequestBus::Events::GetLodType);

            if (m_actorInstance)
            {
                m_previousLodLevel = m_actorInstance->GetLODLevel();
            }
        }

        void SimpleLODComponent::Deactivate()
        {
            AZ::ApplicationTypeQuery appType;
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
            if (appType.IsHeadless())
            {
                return;
            }

            AZ::TickBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();

            AZ::Render::MeshComponentRequestBus::Event(GetEntityId(),
                &AZ::Render::MeshComponentRequestBus::Events::SetLodType,
                m_previousLodType);

            if (m_actorInstance)
            {
                m_actorInstance->SetLODLevel(m_previousLodLevel);
            }
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

        size_t SimpleLODComponent::GetLodByDistance(const AZStd::vector<float>& distances, float distance)
        {
            const size_t max = distances.size();
            for (size_t i = 0; i < max; ++i)
            {
                const float rDistance = distances[i];
                if (distance < rDistance)
                {
                    return i;
                }
            }

            return max - 1;
        }

        void SimpleLODComponent::UpdateLodLevelByDistance(EMotionFX::ActorInstance* actorInstance, const Configuration& configuration, AZ::EntityId entityId)
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
                if (!defaultViewportContext)
                {
                    return;
                }

                const float distance = worldPos.GetDistance(defaultViewportContext->GetCameraTransform().GetTranslation());
                const size_t requestedLod = GetLodByDistance(configuration.m_lodDistances, distance);
                actorInstance->SetLODLevel(requestedLod);

                if (configuration.m_enableLodSampling)
                {
                    const float animGraphSampleRate = configuration.m_lodSampleRates[requestedLod];
                    const float updateRateInSeconds = animGraphSampleRate > 0.0f ? 1.0f / animGraphSampleRate : 0.0f;
                    actorInstance->SetMotionSamplingRate(updateRateInSeconds);
                }
                else if (actorInstance->GetMotionSamplingRate() != 0)
                {
                    actorInstance->SetMotionSamplingRate(0);
                }

                // Disable the automatic mesh LOD level adjustment based on screen space in case a simple LOD component is present.
                // The simple LOD component overrides the mesh LOD level and syncs the skeleton with the mesh LOD level.
                AZ::Render::MeshComponentRequestBus::Event(entityId,
                    &AZ::Render::MeshComponentRequestBus::Events::SetLodType,
                    AZ::RPI::Cullable::LodType::SpecificLod);

                // When setting the actor instance LOD level, a change is just requested and with the next update it will get applied.
                // This means that the current LOD level might differ from the requested one. We need to sync the Atom LOD level with the
                // current LOD level of the actor instance to avoid skinning artifacts. The requested LOD level will be present and applied
                // the following frame.
                const size_t currentLod = actorInstance->GetLODLevel();
                AZ::Render::MeshComponentRequestBus::Event(entityId,
                    &AZ::Render::MeshComponentRequestBus::Events::SetLodOverride,
                    static_cast<AZ::RPI::Cullable::LodOverride>(currentLod));
            }
        }
    } // namespace integration
} // namespace EMotionFX
