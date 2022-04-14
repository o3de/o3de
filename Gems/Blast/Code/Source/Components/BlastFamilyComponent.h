/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Asset/BlastAsset.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>
#include <Blast/BlastDebug.h>
#include <Blast/BlastFamilyComponentBus.h>
#include <Blast/BlastMaterial.h>
#include <Common/BlastInterfaces.h>
#include <Family/ActorRenderManager.h>
#include <Family/BlastFamily.h>
#include <Family/DamageManager.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Scripting/SpawnerComponentBus.h>
#include <NvBlastExtStressSolver.h>

namespace AzPhysics
{
    struct CollisionEvent;
}

namespace Blast
{
    //! Component that handles simulation of the Blast family.
    class BlastFamilyComponent
        : public AZ::Component
        , protected BlastFamilyDamageRequestBus::MultiHandler
        , protected BlastFamilyComponentRequestBus::Handler
        , protected BlastListener
    {
    public:
        AZ_COMPONENT(BlastFamilyComponent, "{88ECE087-C88A-4A83-A83C-477BA9C13221}", AZ::Component);

        BlastFamilyComponent(
            AZ::Data::Asset<BlastAsset> blastAsset, Blast::BlastMaterialId materialId,
            Physics::MaterialId physicsMaterialId, BlastActorConfiguration actorConfiguration);

        BlastFamilyComponent() = default;
        virtual ~BlastFamilyComponent() = default;
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    protected:
        // Lifecycle
        void Spawn();
        void Despawn();

        // BlastFamilyDamageRequestBus overrides ...
        AZ::EntityId GetFamilyId() override;
        void RadialDamage(const AZ::Vector3& position, float minRadius, float maxRadius, float damage) override;
        void CapsuleDamage(
            const AZ::Vector3& position0, const AZ::Vector3& position1, float minRadius, float maxRadius,
            float damage) override;
        void ShearDamage(
            const AZ::Vector3& position, const AZ::Vector3& normal, float minRadius, float maxRadius,
            float damage) override;
        void TriangleDamage(
            const AZ::Vector3& position0, const AZ::Vector3& position1, const AZ::Vector3& position2,
            float damage) override;
        void ImpactSpreadDamage(const AZ::Vector3& position, float minRadius, float maxRadius, float damage) override;
        void StressDamage(const AZ::Vector3& position, const AZ::Vector3& force) override;
        void StressDamage(const BlastActor& blastActor, const AZ::Vector3& position, const AZ::Vector3& force) override;
        void DestroyActor() override;

        // BlastFamilyComponentRequestBus overrides ...
        AZStd::vector<const BlastActor*> GetActors() override;
        AZStd::vector<BlastActorData> GetActorsData() override;
        void FillDebugRenderBuffer(DebugRenderBuffer& debugRenderBuffer, DebugRenderMode debugRenderMode) override;
        void ApplyStressDamage() override;
        void SyncMeshes() override;

    private:
        // BlastListener overrides ...
        // These methods trigger notifications on BlastFamilyComponentNotificationBus.
        void OnActorCreated(const BlastFamily& family, const BlastActor& actor) override;
        void OnActorDestroyed(const BlastFamily& family, const BlastActor& actor) override;

        // Dispatched when two shapes start colliding.
        void OnCollisionBegin(const AzPhysics::CollisionEvent& collisionEvent);

        // Logic processors
        AZStd::unique_ptr<DamageManager> m_damageManager;
        AZStd::unique_ptr<ActorRenderManager> m_actorRenderManager;
        physx::unique_ptr<Nv::Blast::ExtStressSolver> m_solver;
        AZStd::unique_ptr<BlastFamily> m_family;

        // Dependencies
        BlastMeshData* m_meshDataComponent = nullptr;

        // Configurations
        AZ::Data::Asset<BlastAsset> m_blastAsset;
        const BlastMaterialId m_materialId{};
        Physics::MaterialId m_physicsMaterialId;
        const BlastActorConfiguration m_actorConfiguration{};

        bool m_isSpawned = false;
        DebugRenderMode m_debugRenderMode;

        using CollisionHandlersMap = AZStd::unordered_map<AZ::EntityId, AzPhysics::SimulatedBodyEvents::OnCollisionBegin::Handler>;
        using CollisionHandlersMapItr = CollisionHandlersMap::iterator;
        CollisionHandlersMap m_collisionHandlers;
    };
} // namespace Blast
