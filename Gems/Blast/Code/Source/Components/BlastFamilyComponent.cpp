/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/BlastFamilyComponent.h>

#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/MaterialBus.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <Blast/BlastActor.h>
#include <Blast/BlastSystemBus.h>
#include <Material/BlastMaterial.h>
#include <Common/Utils.h>
#include <Components/BlastFamilyComponentNotificationBusHandler.h>
#include <Components/BlastMeshDataComponent.h>
#include <Family/ActorTracker.h>
#include <Family/BlastFamily.h>
#include <NvBlastExtPxActor.h>
#include <NvBlastTkActor.h>
#include <NvBlastTkFamily.h>

namespace Blast
{
    BlastFamilyComponent::BlastFamilyComponent(
        AZ::Data::Asset<BlastAsset> blastAsset,
        AZ::Data::Asset<MaterialAsset> blastMaterialAsset,
        Physics::MaterialId physicsMaterialId,
        const BlastActorConfiguration& actorConfiguration)
        : m_blastAsset(blastAsset)
        , m_blastMaterialAsset(blastMaterialAsset)
        , m_physicsMaterialId(physicsMaterialId)
        , m_actorConfiguration(actorConfiguration)
        , m_debugRenderMode(DebugRenderDisabled)
    {
    }

    void BlastFamilyComponent::Reflect(AZ::ReflectContext* context)
    {
        BlastFamilyComponentNotificationBusHandler::Reflect(context);
        BlastActorConfiguration::Reflect(context);
        BlastActorData::Reflect(context);
        BlastAsset::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BlastFamilyComponent, AZ::Component>()
                ->Version(3)
                ->Field("BlastAsset", &BlastFamilyComponent::m_blastAsset)
                ->Field("BlastMaterialAsset", &BlastFamilyComponent::m_blastMaterialAsset)
                ->Field("PhysicsMaterial", &BlastFamilyComponent::m_physicsMaterialId)
                ->Field("ActorConfiguration", &BlastFamilyComponent::m_actorConfiguration);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<BlastFamilyDamageRequestBus>("BlastFamilyDamageRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "destruction")
                ->Attribute(AZ::Script::Attributes::Category, "Blast")
                ->Event(
                    "Radial Damage", &BlastFamilyDamageRequests::RadialDamage,
                    {{{"position", "The global position of the damage's hit."},
                      {"minRadius", "Damages all chunks/bonds that are in the range [0, minRadius] with full damage.",
                       AZ::BehaviorDefaultValuePtr(aznew AZ::BehaviorDefaultValue(static_cast<float>(0.0)))},
                      {"maxRadius",
                       "Damages all chunks/bonds that are in the range [minRadius, maxRadius] with linearly decreasing "
                       "damage.",
                       AZ::BehaviorDefaultValuePtr(aznew AZ::BehaviorDefaultValue(static_cast<float>(1.0)))},
                      {"damage", "How much damage to deal."}}})
                ->Event(
                    "Capsule Damage", &BlastFamilyDamageRequests::CapsuleDamage,
                    {{{"position0", "The global position of one of the capsule's ends."},
                      {"position1", "The global position of another of the capsule's ends."},
                      {"minRadius", "Damages all chunks/bonds that are in the range [0, minRadius] with full damage.",
                       AZ::BehaviorDefaultValuePtr(aznew AZ::BehaviorDefaultValue(static_cast<float>(0.0)))},
                      {"maxRadius",
                       "Damages all chunks/bonds that are in the range [minRadius, maxRadius] with linearly decreasing "
                       "damage.",
                       AZ::BehaviorDefaultValuePtr(aznew AZ::BehaviorDefaultValue(static_cast<float>(1.0)))},
                      {"damage", "How much damage to deal."}}})
                ->Event(
                    "Shear Damage", &BlastFamilyDamageRequests::ShearDamage,
                    {{{"position", "The global position of the damage's hit."},
                      {"normal", "The normal of the damage's hit."},
                      {"minRadius", "Damages all chunks/bonds that are in the range [0, minRadius] with full damage.",
                       AZ::BehaviorDefaultValuePtr(aznew AZ::BehaviorDefaultValue(static_cast<float>(0.0)))},
                      {"maxRadius",
                       "Damages all chunks/bonds that are in the range [minRadius, maxRadius] with linearly decreasing "
                       "damage.",
                       AZ::BehaviorDefaultValuePtr(aznew AZ::BehaviorDefaultValue(static_cast<float>(1.0)))},
                      {"damage", "How much damage to deal."}}})
                ->Event(
                    "Triangle Damage", &BlastFamilyDamageRequests::TriangleDamage,
                    {{{"position0", "Vertex of the triangle."},
                      {"position1", "Vertex of the triangle."},
                      {"position2", "Vertex of the triangle."},
                      {"damage", "How much damage to deal."}}})
                ->Event(
                    "Impact Spread Damage", &BlastFamilyDamageRequests::ImpactSpreadDamage,
                    {{{"position", "The global position of the damage's hit."},
                      {"minRadius", "Damages all chunks/bonds that are in the range [0, minRadius] with full damage",
                       AZ::BehaviorDefaultValuePtr(aznew AZ::BehaviorDefaultValue(static_cast<float>(0.0)))},
                      {"maxRadius",
                       "Damages all chunks/bonds that are in the range [minRadius, maxRadius] with linearly decreasing "
                       "damage.",
                       AZ::BehaviorDefaultValuePtr(aznew AZ::BehaviorDefaultValue(static_cast<float>(1.0)))},
                      {"damage", "How much damage to deal."}}})
                ->Event(
                    "Stress Damage",
                    static_cast<void (BlastFamilyDamageRequests::*)(const AZ::Vector3&, const AZ::Vector3&)>(
                        &BlastFamilyDamageRequests::StressDamage),
                    {{{"position", "The global position of the damage's hit."},
                      {"force", "The force applied at the position."}}})
                ->Event("Get Family Id", &BlastFamilyDamageRequests::GetFamilyId)
                ->Event("Destroy actor", &BlastFamilyDamageRequests::DestroyActor);

            behaviorContext->EBus<BlastFamilyComponentRequestBus>("BlastFamilyComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "destruction")
                ->Attribute(AZ::Script::Attributes::Category, "Blast")
                ->Event("Get Actors Data", &BlastFamilyComponentRequests::GetActorsData);

            behaviorContext->Class<BlastFamilyComponent>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->RequestBus("BlastFamilyDamageRequestBus")
                ->RequestBus("BlastFamilyComponentRequestBus");
        }
    }

    void BlastFamilyComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("BlastFamilyService"));
    }

    void BlastFamilyComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("BlastFamilyService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void BlastFamilyComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void BlastFamilyComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("BlastMeshDataService"));
    }

    AZStd::vector<const BlastActor*> BlastFamilyComponent::GetActors()
    {
        AZStd::vector<const BlastActor*> actors(
            m_family->GetActorTracker().GetActors().begin(), m_family->GetActorTracker().GetActors().end());
        return actors;
    }

    AZStd::vector<BlastActorData> BlastFamilyComponent::GetActorsData()
    {
        const AZStd::unordered_set<BlastActor*>& familyActors = m_family->GetActorTracker().GetActors();

        AZStd::vector<BlastActorData> actors;
        actors.reserve(familyActors.size());
        for (const auto* actor : familyActors)
        {
            actors.emplace_back(*actor);
        }

        return actors;
    }

    void BlastFamilyComponent::Init()
    {
        m_meshDataComponent = GetEntity()->FindComponent<BlastMeshDataComponent>();
    }

    void BlastFamilyComponent::Activate()
    {
        AZ_PROFILE_FUNCTION(Physics);

        if (const auto blastAssetId = m_blastAsset.GetId();
            blastAssetId.IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(blastAssetId);
        }
        else
        {
            AZ_Warning("BlastFamilyComponent", false, "Blast Family Component created with invalid blast asset.");
            return;
        }

        // If user didn't assign a blast material the default will be used.
        if (const auto blastMaterialAssetId = m_blastMaterialAsset.GetId();
            blastMaterialAssetId.IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(blastMaterialAssetId);
        }

        // Wait for assets to be ready to spawn the blast family
    }

    void BlastFamilyComponent::Deactivate()
    {
        AZ_PROFILE_FUNCTION(Physics);

        AZ::Data::AssetBus::MultiHandler::BusDisconnect();

        Despawn();
    }

    void BlastFamilyComponent::Spawn()
    {
        AZ_PROFILE_FUNCTION(Physics);

        if (m_isSpawned)
        {
            return;
        }

        // Create blast material instance
        if (m_blastMaterialAsset.IsReady())
        {
            m_blastMaterial = AZStd::make_unique<Material>(m_blastMaterialAsset.Get()->GetMaterialConfiguration());
        }
        else
        {
            // Use default material configuration
            m_blastMaterial = AZStd::make_unique<Material>(MaterialConfiguration{});
        }

        auto blastSystem = AZ::Interface<BlastSystemRequests>::Get();

        // Create family
        const BlastFamilyDesc familyDesc
            {*m_blastAsset.Get(),
             this,
             blastSystem->CreateTkGroup(), // Blast system takes care of destroying this group when it's empty.
             m_physicsMaterialId,
             m_blastMaterial.get(),
             AZStd::make_shared<BlastActorFactoryImpl>(),
             EntityProvider::Create(),
             m_actorConfiguration};

        m_family = BlastFamily::Create(familyDesc);

        // Create stress solver
        auto stressSolverSettings =
            m_blastMaterial->GetStressSolverSettings(blastSystem->GetGlobalConfiguration().m_stressSolverIterations);
        // Have to do const cast here, because TkFamily does not give non-const family
        auto solverPtr = Nv::Blast::ExtStressSolver::create(
            const_cast<NvBlastFamily&>(*m_family->GetTkFamily()->getFamilyLL()), stressSolverSettings);
        m_solver = physx::unique_ptr<Nv::Blast::ExtStressSolver>(solverPtr);

        AZStd::shared_ptr<Physics::Material> physicsMaterial;
        Physics::PhysicsMaterialRequestBus::BroadcastResult(
            physicsMaterial,
            &Physics::PhysicsMaterialRequestBus::Events::GetMaterialById,
            m_physicsMaterialId);
        if (!physicsMaterial)
        {
            AZ_Warning("BlastFamilyComponent", false, "Material Id %s was not found, using default material instead.",
                m_physicsMaterialId.GetUuid().ToString<AZStd::string>().c_str());

            Physics::PhysicsMaterialRequestBus::BroadcastResult(
                physicsMaterial,
                &Physics::PhysicsMaterialRequestBus::Events::GetGenericDefaultMaterial);
            AZ_Assert(physicsMaterial, "BlastFamilyComponent: Invalid default physics material");
        }
        m_solver->setAllNodesInfoFromLL(physicsMaterial->GetDensity());

        // Create damage and actor render managers
        m_damageManager = AZStd::make_unique<DamageManager>(m_blastMaterial.get(), m_family->GetActorTracker());

        // Get transform
        AZ::Transform transform = AZ::Transform::Identity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        if (m_meshDataComponent)
        {
            m_actorRenderManager = AZStd::make_unique<ActorRenderManager>(
                AZ::RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::MeshFeatureProcessorInterface>(GetEntityId()),
                m_meshDataComponent, GetEntityId(), m_blastAsset->GetPxAsset()->getChunkCount(), AZ::Vector3(transform.GetUniformScale()));
        }

        // Spawn the family
        m_family->Spawn(transform);

        BlastFamilyDamageRequestBus::MultiHandler::BusConnect(GetEntityId());
        BlastFamilyComponentRequestBus::Handler::BusConnect(GetEntityId());

        m_isSpawned = true;
    }

    void BlastFamilyComponent::Despawn()
    {
        AZ_PROFILE_FUNCTION(Physics);

        if (!m_isSpawned)
        {
            return;
        }

        // Despawn family before reseting members so all the events are properly called
        // and all data is available to be cleaned up accordingly
        m_family->Despawn();

        // collision handlers should have been cleaned up when the family was despawned
        AZ_Assert(m_collisionHandlers.empty(), "%d collision handlers were missing to be deactivated when its blast actors were destroyed", m_collisionHandlers.size());

        // Disconnect from buses before destroying everything
        BlastFamilyDamageRequestBus::MultiHandler::BusDisconnect();
        BlastFamilyComponentRequestBus::Handler::BusDisconnect();

        m_actorRenderManager.reset();
        m_damageManager.reset();
        m_solver.reset();
        m_family.reset();
        m_blastMaterial.reset();

        m_isSpawned = false;
    }

    AZ::EntityId BlastFamilyComponent::GetFamilyId()
    {
        return GetEntityId();
    }

    void BlastFamilyComponent::RadialDamage(
        const AZ::Vector3& position, const float minRadius, const float maxRadius, const float damage)
    {
        if (const auto* entityIdPtr = BlastFamilyDamageRequestBus::GetCurrentBusId();
            !entityIdPtr || *entityIdPtr == GetEntityId())
        {
            m_damageManager->Damage(DamageManager::RadialDamage{}, damage, position, minRadius, maxRadius);
        }
        else if (auto* actor = m_family->GetActorTracker().GetActorById(*entityIdPtr); actor != nullptr)
        {
            m_damageManager->Damage(DamageManager::RadialDamage{}, *actor, damage, position, minRadius, maxRadius);
        }
    }

    void BlastFamilyComponent::CapsuleDamage(
        const AZ::Vector3& position0, const AZ::Vector3& position1, float minRadius, float maxRadius, float damage)
    {
        if (const auto* entityIdPtr = BlastFamilyDamageRequestBus::GetCurrentBusId();
            !entityIdPtr || *entityIdPtr == GetEntityId())
        {
            m_damageManager->Damage(DamageManager::CapsuleDamage{}, damage, position0, position1, minRadius, maxRadius);
        }
        else if (auto* actor = m_family->GetActorTracker().GetActorById(*entityIdPtr); actor != nullptr)
        {
            m_damageManager->Damage(
                DamageManager::CapsuleDamage{}, *actor, damage, position0, position1, minRadius, maxRadius);
        }
    }

    void BlastFamilyComponent::ShearDamage(
        const AZ::Vector3& position, const AZ::Vector3& normal, float minRadius, float maxRadius, float damage)
    {
        if (const auto* entityIdPtr = BlastFamilyDamageRequestBus::GetCurrentBusId();
            !entityIdPtr || *entityIdPtr == GetEntityId())
        {
            m_damageManager->Damage(DamageManager::ShearDamage{}, damage, position, minRadius, maxRadius, normal);
        }
        else if (auto* actor = m_family->GetActorTracker().GetActorById(*entityIdPtr); actor != nullptr)
        {
            m_damageManager->Damage(
                DamageManager::ShearDamage{}, *actor, damage, position, minRadius, maxRadius, normal);
        }
    }

    void BlastFamilyComponent::TriangleDamage(
        const AZ::Vector3& position0, const AZ::Vector3& position1, const AZ::Vector3& position2, float damage)
    {
        if (const auto* entityIdPtr = BlastFamilyDamageRequestBus::GetCurrentBusId();
            !entityIdPtr || *entityIdPtr == GetEntityId())
        {
            m_damageManager->Damage(DamageManager::TriangleDamage{}, damage, position0, position1, position2);
        }
        else if (auto* actor = m_family->GetActorTracker().GetActorById(*entityIdPtr); actor != nullptr)
        {
            m_damageManager->Damage(DamageManager::TriangleDamage{}, *actor, damage, position0, position1, position2);
        }
    }

    void BlastFamilyComponent::ImpactSpreadDamage(
        const AZ::Vector3& position, float minRadius, float maxRadius, float damage)
    {
        if (const auto* entityIdPtr = BlastFamilyDamageRequestBus::GetCurrentBusId();
            !entityIdPtr || *entityIdPtr == GetEntityId())
        {
            m_damageManager->Damage(DamageManager::ImpactSpreadDamage{}, damage, position, minRadius, maxRadius);
        }
        else if (auto* actor = m_family->GetActorTracker().GetActorById(*entityIdPtr); actor != nullptr)
        {
            m_damageManager->Damage(
                DamageManager::ImpactSpreadDamage{}, *actor, damage, position, minRadius, maxRadius);
        }
    }

    void BlastFamilyComponent::StressDamage(const AZ::Vector3& position, const AZ::Vector3& force)
    {
        if (const auto* closestActor = m_family->GetActorTracker().FindClosestActor(position))
        {
            StressDamage(*closestActor, position, force);
        }
    }

    void BlastFamilyComponent::StressDamage(
        const BlastActor& blastActor, const AZ::Vector3& position, const AZ::Vector3& force)
    {
        m_solver->addForce(*blastActor.GetTkActor().getActorLL(), Convert(position), Convert(force));
    }

    void BlastFamilyComponent::DestroyActor()
    {
        if (auto entityIdPtr = BlastFamilyDamageRequestBus::GetCurrentBusId();
            !entityIdPtr || *entityIdPtr == GetEntityId())
        {
            Despawn();
        }
        else
        {
            m_family->DestroyActor(m_family->GetActorTracker().GetActorById(*entityIdPtr));
        }
    }

    void BlastFamilyComponent::OnCollisionBegin(const AzPhysics::CollisionEvent& collisionEvent)
    {
        AZ_PROFILE_FUNCTION(Physics);

        for (const auto* body : {collisionEvent.m_body1, collisionEvent.m_body2})
        {
            auto blastActor = m_family->GetActorTracker().GetActorByBody(body);
            if (!blastActor)
            {
                continue;
            }
            for (auto& contact : collisionEvent.m_contacts)
            {
                auto hitpos = body->GetTransform().GetInverse().TransformPoint(contact.m_position);
                StressDamage(*blastActor, hitpos, contact.m_impulse);
            }
        }
    }

    void BlastFamilyComponent::FillDebugRenderBuffer(
        DebugRenderBuffer& debugRenderBuffer, DebugRenderMode debugRenderMode)
    {
        m_family->FillDebugRender(debugRenderBuffer, debugRenderMode, 1.0f);

        if (!(DebugRenderStressGraph <= debugRenderMode && debugRenderMode <= DebugRenderStressGraphBondsImpulses))
        {
            return;
        }

        for (const BlastActor* blastActor : m_family->GetActorTracker().GetActors())
        {
            const Nv::Blast::TkActor& actor = blastActor->GetTkActor();
            auto lineStartIndex = aznumeric_cast<uint32_t>(debugRenderBuffer.m_lines.size());

            uint32_t nodeCount = actor.getGraphNodeCount();
            if (nodeCount == 0)
            {
                // subsupport chunks don't have graph nodes
                continue;
            }

            std::vector<uint32_t> nodes(nodeCount);
            actor.getGraphNodeIndices(nodes.data(), aznumeric_cast<uint32_t>(nodes.size()));

            if (m_solver)
            {
                const auto buffer = m_solver->fillDebugRender(
                    nodes.data(), aznumeric_cast<uint32_t>(nodes.size()),
                    static_cast<Nv::Blast::ExtStressSolver::DebugRenderMode>(debugRenderMode - DebugRenderStressGraph),
                    1.0);

                if (buffer.lineCount)
                {
                    for (auto i = 0u; i < buffer.lineCount; ++i)
                    {
                        auto& line = buffer.lines[i];
                        AZ::Color color;
                        color.FromU32(line.color0);
                        debugRenderBuffer.m_lines.emplace_back(
                            AZ::Vector3(line.pos0.x, line.pos0.y, line.pos0.z),
                            AZ::Vector3(line.pos1.x, line.pos1.y, line.pos1.z), color);
                    }
                }
            }

            // transform all added lines from local to global
            const AZ::Transform& localToGlobal = blastActor->GetSimulatedBody()->GetTransform();
            for (uint32_t i = lineStartIndex; i < debugRenderBuffer.m_lines.size(); i++)
            {
                DebugLine& line = debugRenderBuffer.m_lines[i];
                line.m_p0 = localToGlobal.TransformPoint(line.m_p0);
                line.m_p1 = localToGlobal.TransformPoint(line.m_p1);
            }
        }
    }

    void BlastFamilyComponent::ApplyStressDamage()
    {
        AZ_PROFILE_FUNCTION(Physics);

        for (auto actor : m_family->GetActorTracker().GetActors())
        {
            auto worldBody = actor->GetSimulatedBody();
            if (actor->IsStatic())
            {
                AZ::Vector3 gravity = AzPhysics::DefaultGravity;
                if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
                {
                    gravity = sceneInterface->GetGravity(worldBody->m_sceneOwner);
                }
                auto localGravity =
                    worldBody->GetTransform().GetRotation().GetInverseFull().TransformVector(gravity);
                m_solver->addGravityForce(*actor->GetTkActor().getActorLL(), Convert(localGravity));
            }
            else
            {
                auto* rigidBody = static_cast<AzPhysics::RigidBody*>(worldBody);
                auto localCenterMass = rigidBody->GetCenterOfMassLocal();
                auto localAngularVelocity =
                    worldBody->GetTransform().GetRotation().GetInverseFull().TransformVector(
                        rigidBody->GetAngularVelocity());
                m_solver->addAngularVelocity(
                    *actor->GetTkActor().getActorLL(), Convert(localCenterMass), Convert(localAngularVelocity));
            }
        }

        m_solver->update();

        if (m_solver->getOverstressedBondCount() > 0)
        {
            NvBlastFractureBuffers commands;
            m_solver->generateFractureCommands(commands);
            if (commands.bondFractureCount > 0)
            {
                m_family->GetTkFamily()->applyFracture(&commands);
            }
        }
    }

    void BlastFamilyComponent::OnActorCreated([[maybe_unused]] const BlastFamily& family, const BlastActor& actor)
    {
        if (m_actorRenderManager)
        {
            m_actorRenderManager->OnActorCreated(actor);
        }

        m_solver->notifyActorCreated(*actor.GetTkActor().getActorLL());

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AZStd::pair<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle> foundBody = physicsSystem->FindAttachedBodyHandleFromEntityId(actor.GetEntity()->GetId());
            if (foundBody.first != AzPhysics::InvalidSceneHandle)
            {
                AzPhysics::SimulatedBodyEvents::OnCollisionBegin::Handler collisionHandler = AzPhysics::SimulatedBodyEvents::OnCollisionBegin::Handler(
                    [this]([[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle, const AzPhysics::CollisionEvent& event)
                    {
                        OnCollisionBegin(event);
                    });
                CollisionHandlersMap::pair_iter_bool newHandlerItr = m_collisionHandlers.emplace(actor.GetEntity()->GetId(), collisionHandler);
                AzPhysics::SimulatedBodyEvents::RegisterOnCollisionBeginHandler(foundBody.first, foundBody.second, newHandlerItr.first->second);
            }
        }
        BlastFamilyDamageRequestBus::MultiHandler::BusConnect(actor.GetEntity()->GetId());
        BlastFamilyComponentNotificationBus::Event(
            GetEntityId(), &BlastFamilyComponentNotifications::OnActorCreated, actor);
    }

    void BlastFamilyComponent::OnActorDestroyed([[maybe_unused]] const BlastFamily& family, const BlastActor& actor)
    {
        BlastFamilyComponentNotificationBus::Event(
            GetEntityId(), &BlastFamilyComponentNotifications::OnActorDestroyed, actor);
        BlastFamilyDamageRequestBus::MultiHandler::BusDisconnect(actor.GetEntity()->GetId());

        if (CollisionHandlersMapItr foundHandler = m_collisionHandlers.find(actor.GetEntity()->GetId());
            foundHandler != m_collisionHandlers.end())
        {
            foundHandler->second.Disconnect();
            m_collisionHandlers.erase(foundHandler);
        }

        m_solver->notifyActorDestroyed(*actor.GetTkActor().getActorLL());

        if (m_actorRenderManager)
        {
            m_actorRenderManager->OnActorDestroyed(actor);
        }
    }

    // Update positions of entities with render meshes corresponding to their right dynamic bodies.
    void BlastFamilyComponent::SyncMeshes()
    {
        AZ_PROFILE_FUNCTION(Physics);

        if (m_actorRenderManager)
        {
            m_actorRenderManager->SyncMeshes();
        }
    }

    void BlastFamilyComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_blastAsset == asset)
        {
            m_blastAsset = asset;
        }

        const bool expectingMaterialAsset = m_blastMaterialAsset.GetId().IsValid();
        if (expectingMaterialAsset && m_blastMaterialAsset == asset)
        {
            m_blastMaterialAsset = asset;
        }

        if (m_blastAsset.IsReady() &&
            (!expectingMaterialAsset || m_blastMaterialAsset.IsReady()))
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            Spawn();
        }
    }

    void BlastFamilyComponent::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_blastAsset == asset)
        {
            AZ_Warning("BlastFamilyComponent", false, "Blast Family Component has an invalid blast asset.");
        }
        else if (m_blastMaterialAsset == asset)
        {
            AZ_Warning("BlastFamilyComponent", false, "Blast Family Component has an invalid blast material asset. Default material will be used.");

            // Set asset to an invalid id to know it's not expected to have a material asset and use the default material instead.
            m_blastMaterialAsset = {};

            if (m_blastAsset.IsReady())
            {
                AZ::Data::AssetBus::MultiHandler::BusDisconnect();

                Spawn();
            }
        }
    }
} // namespace Blast
