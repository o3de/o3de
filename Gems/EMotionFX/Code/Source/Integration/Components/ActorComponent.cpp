/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>

#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Visibility/BoundsBus.h>

#include <LmbrCentral/Animation/AttachmentComponentBus.h>

#include <Integration/Components/ActorComponent.h>
#include <Integration/Rendering/RenderBackendManager.h>

#include <EMotionFX/Source/Transform.h>
#include <EMotionFX/Source/RagdollInstance.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/AttachmentSkin.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/AttachmentNode.h>

#include <MCore/Source/AzCoreConversions.h>

#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        class ActorComponentNotificationBehaviorHandler
            : public ActorComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(ActorComponentNotificationBehaviorHandler, "{4631E2E1-62CB-451D-A6E3-CC40501879AE}", AZ::SystemAllocator,
                OnActorInstanceCreated, OnActorInstanceDestroyed);

            void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override
            {
                Call(FN_OnActorInstanceCreated, actorInstance);
            }

            void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override
            {
                Call(FN_OnActorInstanceDestroyed, actorInstance);
            }
        };

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::BoundingBoxConfiguration::Set(ActorInstance* actorInstance) const
        {
            actorInstance->SetExpandBoundsBy(m_expandBy * 0.01f); // Normalize percentage for internal use. (1% == 0.01f)

            if (m_autoUpdateBounds)
            {
                actorInstance->SetupAutoBoundsUpdate(m_updateTimeFrequency, m_boundsType, m_updateItemFrequency);
            }
            else
            {
                actorInstance->SetBoundsUpdateType(m_boundsType);
                actorInstance->SetBoundsUpdateEnabled(false);
            }
        }

        void ActorComponent::BoundingBoxConfiguration::SetAndUpdate(ActorInstance* actorInstance) const
        {
            Set(actorInstance);

            const AZ::u32 updateFrequency = actorInstance->GetBoundsUpdateEnabled() ? actorInstance->GetBoundsUpdateItemFrequency() : 1;
            const ActorInstance::EBoundsType boundUpdateType = actorInstance->GetBoundsUpdateType();

            actorInstance->UpdateBounds(actorInstance->GetLODLevel(), boundUpdateType, updateFrequency);
        }

        void ActorComponent::BoundingBoxConfiguration::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BoundingBoxConfiguration>()
                    ->Version(2, [](AZ::SerializeContext& sc, AZ::SerializeContext::DataElementNode& node)
                    {
                        if (node.GetVersion() < 2)
                        {
                            // m_boundsType used to be an enum class with `int' underlying type, is now `u8'
                            static const char* m_boundsType_name = "m_boundsType";
                            static AZ::Crc32 m_boundsType_nameCrc(m_boundsType_name);

                            int m_boundsType_as_int;
                            if (!node.GetChildData(m_boundsType_nameCrc, m_boundsType_as_int))
                            {
                                return false;
                            }
                            if (!node.RemoveElementByName(m_boundsType_nameCrc)) return false;
                            if (node.AddElementWithData(sc, m_boundsType_name, (AZ::u8)m_boundsType_as_int) == -1) return false;
                        }
                        return true;
                    })
                    ->Field("m_boundsType", &BoundingBoxConfiguration::m_boundsType)
                    ->Field("m_autoUpdateBounds", &BoundingBoxConfiguration::m_autoUpdateBounds)
                    ->Field("m_updateTimeFrequency", &BoundingBoxConfiguration::m_updateTimeFrequency)
                    ->Field("m_updateItemFrequency", &BoundingBoxConfiguration::m_updateItemFrequency)
                    ->Field("expandBy", &BoundingBoxConfiguration::m_expandBy)
                    ;
            }
        }

        AZ::Crc32 ActorComponent::BoundingBoxConfiguration::GetVisibilityAutoUpdate() const
        {
            return m_boundsType != EMotionFX::ActorInstance::BOUNDS_STATIC_BASED ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        AZ::Crc32 ActorComponent::BoundingBoxConfiguration::GetVisibilityAutoUpdateSettings() const
        {
            if (m_boundsType == EMotionFX::ActorInstance::BOUNDS_STATIC_BASED || m_autoUpdateBounds == false)
            {
                return AZ::Edit::PropertyVisibility::Hide;
            }

            return AZ::Edit::PropertyVisibility::Show;
        }


        //////////////////////////////////////////////////////////////////////////
        AZ_ENUM_DEFINE_REFLECT_UTILITIES(ActorRenderFlags);

        void ActorComponent::Configuration::Reflect(AZ::ReflectContext* context)
        {
            BoundingBoxConfiguration::Reflect(context);

            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                ActorRenderFlagsReflect(*serializeContext);

                serializeContext->Class<Configuration>()
                    ->Version(8)
                    ->Field("ActorAsset", &Configuration::m_actorAsset)
                    ->Field("AttachmentType", &Configuration::m_attachmentType)
                    ->Field("AttachmentTarget", &Configuration::m_attachmentTarget)
                    ->Field("SkinningMethod", &Configuration::m_skinningMethod)
                    ->Field("LODLevel", &Configuration::m_lodLevel)
                    ->Field("BoundingBoxConfig", &Configuration::m_bboxConfig)
                    ->Field("ForceJointsUpdateOOV", &Configuration::m_forceUpdateJointsOOV)
                    ->Field("RenderFlags", &Configuration::m_renderFlags)
                    ->Field("ExcludeFromReflectionCubeMaps", &Configuration::m_excludeFromReflectionCubeMaps)
                    ->Field("LightingChannelConfig", &Configuration::m_lightingChannelConfig)
                    ->Field("RayTracingEnabled", &Configuration::m_rayTracingEnabled)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                Configuration::Reflect(context);

                serializeContext->Class<ActorComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Configuration", &ActorComponent::m_configuration)
                ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Enum<EMotionFX::Integration::Space>("Space", "The transformation space.")
                        ->Value("Local Space", Space::LocalSpace)
                        ->Value("Model Space", Space::ModelSpace)
                        ->Value("World Space", Space::WorldSpace);
                }
            }

            auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<ActorComponentRequestBus>("ActorComponentRequestBus")
                    ->Event("GetJointIndexByName", &ActorComponentRequestBus::Events::GetJointIndexByName)
                    ->Event("GetJointTransform", &ActorComponentRequestBus::Events::GetJointTransform)
                    ->Event("AttachToEntity", &ActorComponentRequestBus::Events::AttachToEntity)
                    ->Event("DetachFromEntity", &ActorComponentRequestBus::Events::DetachFromEntity)
                    ->Event("GetRenderCharacter", &ActorComponentRequestBus::Events::GetRenderCharacter)
                    ->Event("SetRenderCharacter", &ActorComponentRequestBus::Events::SetRenderCharacter)
                    ->Event("GetRenderActorVisible", &ActorComponentRequestBus::Events::GetRenderActorVisible)
                    ->Event("SetRayTracingEnabled", &ActorComponentRequestBus::Events::SetRayTracingEnabled)
                    ->Event("EnableInstanceUpdate", &ActorComponentRequestBus::Events::EnableInstanceUpdate)
                    ->VirtualProperty("RenderCharacter", "GetRenderCharacter", "SetRenderCharacter")
                ;

                behaviorContext->Class<ActorComponent>()->RequestBus("ActorComponentRequestBus");

                behaviorContext->EBus<ActorComponentNotificationBus>("ActorComponentNotificationBus")
                    ->Handler<ActorComponentNotificationBehaviorHandler>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::List)
                ;
            }
        }

        void ActorComponent::SetActorAsset(AZ::Data::Asset<ActorAsset> actorAsset)
        {
            m_configuration.m_actorAsset = actorAsset;
            CheckActorCreation();
        }

        void ActorComponent::EnableInstanceUpdate(bool enable)
        {
            if (m_actorInstance)
            {
                m_actorInstance->SetIsEnabled(enable);
            }
            else
            {
                AZ_ErrorOnce("EMotionFX", false, "Cannot enable the actor instance update because actor instance haven't been created.");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        ActorComponent::ActorComponent(const Configuration* configuration)
            : m_sceneFinishSimHandler([this]([[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                float fixedDeltatime)
                {
                    if (m_actorInstance)
                    {
                        m_actorInstance->PostPhysicsUpdate(fixedDeltatime);
                    }
                }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Animation))
        {
            if (configuration)
            {
                m_configuration = *configuration;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        ActorComponent::~ActorComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::Activate()
        {
            m_actorInstance.reset();

            auto& cfg = m_configuration;

            if (cfg.m_actorAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::Handler::BusDisconnect();
                AZ::Data::AssetBus::Handler::BusConnect(cfg.m_actorAsset.GetId());
                cfg.m_actorAsset.QueueLoad();
            }

            AZ::TickBus::Handler::BusConnect();

            const AZ::EntityId entityId = GetEntityId();
            ActorComponentRequestBus::Handler::BusConnect(entityId);
            LmbrCentral::AttachmentComponentNotificationBus::Handler::BusConnect(entityId);
            AzFramework::CharacterPhysicsDataRequestBus::Handler::BusConnect(entityId);
            AzFramework::RagdollPhysicsNotificationBus::Handler::BusConnect(entityId);
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);

            if (cfg.m_attachmentTarget.IsValid())
            {
                AttachToEntity(cfg.m_attachmentTarget, cfg.m_attachmentType);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::Deactivate()
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            AzFramework::RagdollPhysicsNotificationBus::Handler::BusDisconnect();
            AzFramework::CharacterPhysicsDataRequestBus::Handler::BusDisconnect();
            m_sceneFinishSimHandler.Disconnect();
            ActorComponentRequestBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();
            LmbrCentral::AttachmentComponentNotificationBus::Handler::BusDisconnect();
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusDisconnect();

            DestroyActor();
            m_configuration.m_actorAsset.Release();
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::AttachToEntity(AZ::EntityId targetEntityId, [[maybe_unused]] AttachmentType attachmentType)
        {
            if (targetEntityId.IsValid() && targetEntityId != GetEntityId())
            {
                m_attachmentTargetEntityId = targetEntityId;

                ActorComponentNotificationBus::Handler::BusDisconnect();
                ActorComponentNotificationBus::Handler::BusConnect(targetEntityId);

                AZ::TransformNotificationBus::MultiHandler::BusConnect(targetEntityId);

                // There's no guarantee that we will receive a on transform change call for the target entity because of the entity activate order.
                // Enforce a transform query on target to get the correct initial transform.
                AZ::Transform transform;
                AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM); // default to using our own TM
                AZ::TransformBus::EventResult(transform, targetEntityId, &AZ::TransformBus::Events::GetWorldTM); // attempt to get target's TM
                AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTM, transform); // set our TM
            }
            else
            {
                DetachFromEntity();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::DetachFromEntity()
        {
            if (!m_actorInstance)
            {
                return;
            }

            ActorInstance* attachedTo = m_actorInstance->GetAttachedTo();
            if (attachedTo)
            {
                attachedTo->RemoveAttachment(m_actorInstance.get());
                AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetParent, m_attachmentPreviousParent);
                AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetLocalTM, AZ::Transform::CreateIdentity());

                AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_attachmentTargetEntityId);
                m_attachmentTargetEntityId.SetInvalid();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        bool ActorComponent::GetRenderCharacter() const
        {
            return AZ::RHI::CheckBitsAny(m_configuration.m_renderFlags, ActorRenderFlags::Solid);
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::SetRenderCharacter(bool enable)
        {
            if (enable)
            {
                m_configuration.m_renderFlags |= ActorRenderFlags::Solid;
            }
            else
            {
                m_configuration.m_renderFlags &= ~ActorRenderFlags::Solid;
            }

            if (m_renderActorInstance)
            {
                m_renderActorInstance->SetIsVisible(enable);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        bool ActorComponent::GetRenderActorVisible() const
        {
            if (m_renderActorInstance)
            {
                return m_renderActorInstance->IsVisible();
            }
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::SetRayTracingEnabled(bool enabled)
        {
            if (m_renderActorInstance)
            {
                return m_renderActorInstance->SetRayTracingEnabled(enabled);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        SkinningMethod ActorComponent::GetSkinningMethod() const
        {
            return m_configuration.m_skinningMethod;
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_configuration.m_actorAsset = asset;
            AZ_Assert(m_configuration.m_actorAsset.IsReady() && m_configuration.m_actorAsset->GetActor(), "Actor asset should be loaded and actor valid.");

            // We'll defer actor creation until the next tick on the tick bus. This is because OnAssetReady() can sometimes get
            // triggered while in the middle of the render tick, since the rendering system sometimes contains blocking loads
            // which will still process any pending OnAssetReady() commands while waiting. If that occurs, the actor creation
            // would generate errors from trying to create a rendering actor while in the middle of processing the rendering data.
            // We can avoid the problem by just always waiting until the next tick to create the actor.
            m_processLoadedAsset = true;
        }

        void ActorComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        bool ActorComponent::IsPhysicsSceneSimulationFinishEventConnected() const
        {
            return m_sceneFinishSimHandler.IsConnected();
        }

        void ActorComponent::SetRenderFlag(ActorRenderFlags renderFlags)
        {
            m_configuration.m_renderFlags = renderFlags;
        }

        void ActorComponent::CheckActorCreation()
        {
            DestroyActor();

            // Create actor instance.
            auto* actorAsset = m_configuration.m_actorAsset.GetAs<ActorAsset>();
            AZ_Error("EMotionFX", actorAsset, "Actor asset is not valid.");
            if (!actorAsset)
            {
                return;
            }

            m_actorInstance = actorAsset->CreateInstance(GetEntity());
            if (!m_actorInstance)
            {
                AZ_Error("EMotionFX", actorAsset, "Failed to create actor instance.");
                return;
            }

            ActorComponentNotificationBus::Event(
                GetEntityId(),
                &ActorComponentNotificationBus::Events::OnActorInstanceCreated,
                m_actorInstance.get());

            m_actorInstance->SetLODLevel(m_configuration.m_lodLevel);
            m_actorInstance->SetLightingChannelMask(m_configuration.m_lightingChannelConfig.GetLightingChannelMask());

            // Setup initial transform and listen for transform changes.
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            OnTransformChanged(transform, transform);
            AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());

            m_actorInstance->UpdateWorldTransform();
            // Set bounds update mode and compute bbox first time
            m_configuration.m_bboxConfig.SetAndUpdate(m_actorInstance.get());
            m_actorInstance->UpdateBounds(0, ActorInstance::EBoundsType::BOUNDS_STATIC_BASED);

            // Creating the render actor AFTER both actor asset and mesh asset loaded.
            RenderBackend* renderBackend = AZ::Interface<RenderBackendManager>::Get()->GetRenderBackend();
            if (renderBackend)
            {
                actorAsset->InitRenderActor();

                // If there is already a RenderActorInstance, destroy it before creating the new one so there are not two instances potentially handling events for the same entityId
                m_renderActorInstance.reset(nullptr);
                // Create the new RenderActorInstance
                m_renderActorInstance.reset(renderBackend->CreateActorInstance(GetEntityId(),
                    m_actorInstance,
                    m_configuration.m_actorAsset,
                    m_configuration.m_skinningMethod,
                    transform,
                    m_configuration.m_rayTracingEnabled));

                if (m_renderActorInstance)
                {
                    m_renderActorInstance->SetIsVisible(AZ::RHI::CheckBitsAny(m_configuration.m_renderFlags, ActorRenderFlags::Solid));
                    m_renderActorInstance->SetExcludeFromReflectionCubeMaps(m_configuration.m_excludeFromReflectionCubeMaps);
                }
            }

            // Remember the parent entity before we re-parent (attach) it.
            AZ::TransformBus::EventResult(m_attachmentPreviousParent, GetEntityId(), &AZ::TransformBus::Events::GetParentId);

            // Reattach all attachments
            for (AZ::EntityId& attachment : m_attachments)
            {
                LmbrCentral::AttachmentComponentRequestBus::Event(attachment, &LmbrCentral::AttachmentComponentRequestBus::Events::Reattach, true);
            }

            const AZ::EntityId entityId = GetEntityId();
            LmbrCentral::AttachmentComponentRequestBus::Event(entityId, &LmbrCentral::AttachmentComponentRequestBus::Events::Reattach, true);

            CheckAttachToEntity();

            Physics::RagdollConfiguration ragdollConfiguration;
            [[maybe_unused]] bool ragdollConfigValid = GetRagdollConfiguration(ragdollConfiguration);
            AZ_Assert(ragdollConfigValid, "Ragdoll Configuration is not valid");
            AzFramework::CharacterPhysicsDataNotificationBus::Event(entityId, &AzFramework::CharacterPhysicsDataNotifications::OnRagdollConfigurationReady, ragdollConfiguration);
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::CheckAttachToEntity()
        {
            // Attach to the target actor if we're both ready.
            if (m_actorInstance)
            {
                if (m_attachmentTargetEntityId.IsValid())
                {
                    // Create the attachment if the target instance is already created.
                    // Otherwise, listen to the actor instance creation event.
                    ActorInstance* targetActorInstance = nullptr;
                    ActorComponentRequestBus::EventResult(targetActorInstance, m_attachmentTargetEntityId, &ActorComponentRequestBus::Events::GetActorInstance);
                    if (targetActorInstance)
                    {
                        DetachFromEntity();

                        // Make sure we don't generate some circular loop by attaching to each other.
                        if (!targetActorInstance->CheckIfCanHandleAttachment(m_actorInstance.get()))
                        {
                            AZ_Error("EMotionFX", false, "You cannot attach to yourself or create circular dependencies!\n");
                            return;
                        }

                        // Remember the parent entity before we re-parent (attach) it.
                        AZ::TransformBus::EventResult(m_attachmentPreviousParent, GetEntityId(), &AZ::TransformBus::Events::GetParentId);

                        // Create the attachment.
                        AZ_Assert(m_configuration.m_attachmentType == AttachmentType::SkinAttachment, "Expected a skin attachment.");
                        Attachment* attachment = AttachmentSkin::Create(targetActorInstance, m_actorInstance.get());
                        m_actorInstance->SetLocalSpaceTransform(Transform::CreateIdentity());
                        targetActorInstance->AddAttachment(attachment);
                        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetParent, targetActorInstance->GetEntityId());
                        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetLocalTM, AZ::Transform::CreateIdentity());
                    }
                }
                else
                {
                    DetachFromEntity();
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::DestroyActor()
        {
            DetachFromEntity();

            m_renderActorInstance.reset();

            if (m_actorInstance)
            {

                ActorComponentNotificationBus::Event(
                    GetEntityId(),
                    &ActorComponentNotificationBus::Events::OnActorInstanceDestroyed,
                    m_actorInstance.get());

                m_actorInstance.reset();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
        {
            AZ_UNUSED(local);

            const AZ::EntityId* busIdPtr = AZ::TransformNotificationBus::GetCurrentBusId();
            if (!busIdPtr || *busIdPtr == GetEntityId()) // Our own entity has moved.
            {
                // If we're not attached to another actor, keep the EMFX root in sync with any external changes to the entity's transform.
                if (m_actorInstance)
                {
                    const Transform localTransform = m_actorInstance->GetParentWorldSpaceTransform().Inversed() * Transform(world);
                    m_actorInstance->SetLocalSpacePosition(localTransform.m_position);
                    m_actorInstance->SetLocalSpaceRotation(localTransform.m_rotation);

                    // Disable updating the scale to prevent feedback from adding up.
                    // We need to find a better way to handle this or to prevent this feedback loop.
                    EMFX_SCALECODE
                    (
                        m_actorInstance->SetLocalSpaceScale(localTransform.m_scale);
                    )
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            AZ_PROFILE_FUNCTION(Animation);

            // If we've got an asset that finished loading (denoted by an OnAssetReady() call), create the actor instance here.
            if (m_processLoadedAsset)
            {
                CheckActorCreation();
                m_processLoadedAsset = false;
            }

            if (!m_actorInstance || !m_actorInstance->GetIsEnabled())
            {
                return;
            }

            if (m_renderActorInstance)
            {
                m_renderActorInstance->OnTick(deltaTime);
                m_renderActorInstance->UpdateBounds();
                AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->RefreshEntityLocalBoundsUnion(GetEntityId());

                const bool isInCameraFrustum = m_renderActorInstance->IsInCameraFrustum();
                const bool renderActorSolid = AZ::RHI::CheckBitsAny(m_configuration.m_renderFlags, ActorRenderFlags::Solid);
                m_renderActorInstance->SetIsVisible(isInCameraFrustum && renderActorSolid);

                // Optimization: Set the actor instance invisible when character is out of camera view. This will stop the joint transforms update, except the root joint.
                // Calling it after the bounds on the render actor updated.
                if (!m_configuration.m_forceUpdateJointsOOV)
                {
                    // Update the skeleton in case solid mesh rendering or any of the debug visualizations are enabled and the character is in the camera frustum.
                    const bool updateTransforms = AZ::RHI::CheckBitsAny(m_configuration.m_renderFlags, s_requireUpdateTransforms);
                    m_actorInstance->SetIsVisible(isInCameraFrustum && updateTransforms);
                }
            }
        }

        int ActorComponent::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }

        void ActorComponent::DisplayEntityViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
            [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
        {
            if (m_renderActorInstance)
            {
                m_renderActorInstance->DebugDraw(m_configuration.m_renderFlags);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorComponent::OnActorInstanceCreated(ActorInstance* actorInstance)
        {
            auto it = AZStd::find(m_attachments.begin(), m_attachments.end(), actorInstance->GetEntityId());
            if (it != m_attachments.end())
            {
                if (m_actorInstance)
                {
                    LmbrCentral::AttachmentComponentRequestBus::Event(actorInstance->GetEntityId(), &LmbrCentral::AttachmentComponentRequestBus::Events::Reattach, true);
                }
            }
            else
            {
                CheckAttachToEntity();
            }
        }

        void ActorComponent::OnActorInstanceDestroyed([[maybe_unused]] ActorInstance* actorInstance)
        {
            DetachFromEntity();
        }

        //////////////////////////////////////////////////////////////////////////
        bool ActorComponent::GetRagdollConfiguration(Physics::RagdollConfiguration& ragdollConfiguration) const
        {
            if (!m_actorInstance)
            {
                return false;
            }

            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actorInstance->GetActor()->GetPhysicsSetup();
            ragdollConfiguration = physicsSetup->GetRagdollConfig();
            return true;
        }

        AZStd::string ActorComponent::GetParentNodeName(const AZStd::string& childName) const
        {
            if (!m_actorInstance)
            {
                return AZStd::string();
            }

            const Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
            Node* childNode = skeleton->FindNodeByName(childName);
            if (childNode)
            {
                const Node* parentNode = childNode->GetParentNode();
                if (parentNode)
                {
                    return parentNode->GetNameString();
                }
            }

            return AZStd::string();
        }

        //////////////////////////////////////////////////////////////////////////
        Physics::RagdollState ActorComponent::GetBindPose(const Physics::RagdollConfiguration& config) const
        {
            Physics::RagdollState physicsPose;

            if (!m_actorInstance)
            {
                return physicsPose;
            }

            const Actor* actor = m_actorInstance->GetActor();
            const Skeleton* skeleton = actor->GetSkeleton();
            const Pose* emfxPose = actor->GetBindPose();

            size_t numNodes = config.m_nodes.size();
            physicsPose.resize(numNodes);

            for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
            {
                const char* nodeName = config.m_nodes[nodeIndex].m_debugName.data();
                Node* emfxNode = skeleton->FindNodeByName(nodeName);
                AZ_Error("EMotionFX", emfxNode, "Could not find bind pose for node %s", nodeName);
                if (emfxNode)
                {
                    const Transform& nodeTransform = emfxPose->GetModelSpaceTransform(emfxNode->GetNodeIndex());
                    physicsPose[nodeIndex].m_position = nodeTransform.m_position;
                    physicsPose[nodeIndex].m_orientation = nodeTransform.m_rotation;
                }
            }

            return physicsPose;
        }

        void ActorComponent::OnRagdollActivated()
        {
            Physics::Ragdoll* ragdoll;
            AzFramework::RagdollPhysicsRequestBus::EventResult(ragdoll, m_entity->GetId(), &AzFramework::RagdollPhysicsRequestBus::Events::GetRagdoll);
            if (ragdoll && m_actorInstance)
            {
                m_actorInstance->SetRagdoll(ragdoll);

                RagdollInstance* ragdollInstance = m_actorInstance->GetRagdollInstance();
                AZ_Assert(ragdollInstance, "As the ragdoll passed in ActorInstance::SetRagdoll() is valid, a valid ragdoll instance is expected to exist.");
                if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
                {
                    sceneInterface->RegisterSceneSimulationFinishHandler(ragdollInstance->GetRagdollSceneHandle(), m_sceneFinishSimHandler);
                }
            }
        }

        void ActorComponent::OnRagdollDeactivated()
        {
            if (m_actorInstance)
            {
                m_sceneFinishSimHandler.Disconnect();
                m_actorInstance->SetRagdoll(nullptr);
            }
        }

        size_t ActorComponent::GetNumJoints() const
        {
            AZ_Assert(m_actorInstance, "The actor instance needs to be valid.");

            return m_actorInstance->GetActor()->GetNumNodes();
        }

        size_t ActorComponent::GetJointIndexByName(const char* name) const
        {
            AZ_Assert(m_actorInstance, "The actor instance needs to be valid.");

            Node* node = m_actorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase(name);
            if (node)
            {
                return static_cast<size_t>(node->GetNodeIndex());
            }

            return ActorComponentRequests::s_invalidJointIndex;
        }


        AZ::Transform ActorComponent::GetJointTransform(size_t jointIndex, Space space) const
        {
            AZ_Assert(m_actorInstance, "The actor instance needs to be valid.");

            const size_t index = jointIndex;
            const size_t numNodes = m_actorInstance->GetActor()->GetNumNodes();

            AZ_Error("EMotionFX", index < numNodes, "GetJointTransform: The joint index %zu is out of bounds [0;%zu]. Entity: %s",
                index, numNodes, GetEntity()->GetName().c_str());

            if (index >= numNodes)
            {
                return AZ::Transform::CreateIdentity();
            }

            Pose* currentPose = m_actorInstance->GetTransformData()->GetCurrentPose();
            switch (space)
            {
            case Space::LocalSpace:
            {
                return MCore::EmfxTransformToAzTransform(currentPose->GetLocalSpaceTransform(index));
            }

            case Space::ModelSpace:
            {
                return MCore::EmfxTransformToAzTransform(currentPose->GetModelSpaceTransform(index));
            }

            case Space::WorldSpace:
            {
                return MCore::EmfxTransformToAzTransform(currentPose->GetWorldSpaceTransform(index));
            }

            default:
                AZ_Assert(false, "Unsupported space in GetJointTransform!");
            }

            return AZ::Transform::CreateIdentity();
        }

        void ActorComponent::GetJointTransformComponents(size_t jointIndex, Space space, AZ::Vector3& outPosition, AZ::Quaternion& outRotation, AZ::Vector3& outScale) const
        {
            AZ_Assert(m_actorInstance, "The actor instance needs to be valid.");

            const size_t index = jointIndex;
            const size_t numNodes = m_actorInstance->GetActor()->GetNumNodes();

            AZ_Error("EMotionFX", index < numNodes, "GetJointTransformComponents: The joint index %zu is out of bounds [0;%zu]. Entity: %s",
                index, numNodes, GetEntity()->GetName().c_str());

            if (index >= numNodes)
            {
                return;
            }

            Pose* currentPose = m_actorInstance->GetTransformData()->GetCurrentPose();

            switch (space)
            {
            case Space::LocalSpace:
            {
                const Transform& localTransform = currentPose->GetLocalSpaceTransform(index);
                outPosition = localTransform.m_position;
                outRotation = localTransform.m_rotation;
                EMFX_SCALECODE
                (
                    outScale = localTransform.m_scale;
                )
                return;
            }

            case Space::ModelSpace:
            {
                const Transform& modelTransform = currentPose->GetModelSpaceTransform(index);
                outPosition = modelTransform.m_position;
                outRotation = modelTransform.m_rotation;
                EMFX_SCALECODE
                (
                    outScale = modelTransform.m_scale;
                )
                return;
            }

            case Space::WorldSpace:
            {
                const Transform worldTransform = currentPose->GetWorldSpaceTransform(index);
                outPosition = worldTransform.m_position;
                outRotation = worldTransform.m_rotation;
                EMFX_SCALECODE
                (
                    outScale = worldTransform.m_scale;
                )
                return;
            }

            default:
            {
                AZ_Assert(false, "Unsupported space in GetJointTransform!");
                outPosition = AZ::Vector3::CreateZero();
                outRotation = AZ::Quaternion::CreateIdentity();
                outScale = AZ::Vector3::CreateOne();
            }
            }
        }

        Physics::AnimationConfiguration* ActorComponent::GetPhysicsConfig() const
        {
            if (m_actorInstance)
            {
                Actor* actor = m_actorInstance->GetActor();
                const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
                if (physicsSetup)
                {
                    return &physicsSetup->GetConfig();
                }
            }

            return nullptr;
        }

        // The entity has attached to the target.
        void ActorComponent::OnAttached(AZ::EntityId attachedEntityId)
        {
            const AZ::EntityId* busIdPtr = LmbrCentral::AttachmentComponentNotificationBus::GetCurrentBusId();
            if (busIdPtr)
            {
                const auto result = AZStd::find(m_attachments.begin(), m_attachments.end(), attachedEntityId);
                if (result == m_attachments.end())
                {
                    m_attachments.emplace_back(attachedEntityId);
                }
                else
                {
                    return;
                }
            }

            if (!m_actorInstance)
            {
                return;
            }

            ActorInstance* targetActorInstance = nullptr;
            ActorComponentRequestBus::EventResult(targetActorInstance, attachedEntityId, &ActorComponentRequestBus::Events::GetActorInstance);

            const char* jointName = nullptr;
            LmbrCentral::AttachmentComponentRequestBus::EventResult(jointName, attachedEntityId, &LmbrCentral::AttachmentComponentRequestBus::Events::GetJointName);
            if (targetActorInstance)
            {
                Node* node = jointName ? m_actorInstance->GetActor()->GetSkeleton()->FindNodeByName(jointName) : m_actorInstance->GetActor()->GetSkeleton()->GetNode(0);
                if (node)
                {
                    const size_t jointIndex = node->GetNodeIndex();
                    Attachment* attachment = AttachmentNode::Create(m_actorInstance.get(), jointIndex, targetActorInstance, true /* Managed externally, by this component. */);
                    m_actorInstance->AddAttachment(attachment);
                }
            }
        }

        // The entity is detaching from the target.
        void ActorComponent::OnDetached(AZ::EntityId targetId)
        {
            // Remove the targetId from the attachment list
            const AZ::EntityId* busIdPtr = LmbrCentral::AttachmentComponentNotificationBus::GetCurrentBusId();
            if (busIdPtr)
            {
                m_attachments.erase(AZStd::remove(m_attachments.begin(), m_attachments.end(), targetId), m_attachments.end());
            }

            if (!m_actorInstance)
            {
                return;
            }

            ActorInstance* targetActorInstance = nullptr;
            ActorComponentRequestBus::EventResult(targetActorInstance, targetId, &ActorComponentRequestBus::Events::GetActorInstance);
            if (targetActorInstance)
            {
                m_actorInstance->RemoveAttachment(targetActorInstance);
            }
        }
    } // namespace Integration
} // namespace EMotionFX
