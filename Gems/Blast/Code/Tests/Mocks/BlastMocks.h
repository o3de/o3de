/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>

#include <Actor/EntityProvider.h>
#include <Blast/BlastActor.h>
#include <Blast/BlastSystemBus.h>
#include <Components/BlastMeshDataComponent.h>
#include <Family/ActorTracker.h>
#include <Family/BlastFamily.h>
#include <NvBlastExtPxAsset.h>
#include <NvBlastTkActor.h>
#include <NvBlastTkAsset.h>
#include <NvBlastTkFamily.h>
#include <NvBlastTkGroup.h>

#include <gmock/gmock.h>

namespace Blast
{
    class FastScopedAllocatorsBase
    {
    public:
        FastScopedAllocatorsBase()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        ~FastScopedAllocatorsBase()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };

    class BlastFamily;

    class FakeExtPxAsset : public Nv::Blast::ExtPxAsset
    {
    public:
        virtual ~FakeExtPxAsset() = default;

        FakeExtPxAsset(
            const NvBlastActorDesc& desc, AZStd::vector<Nv::Blast::ExtPxChunk> chunks = {},
            AZStd::vector<Nv::Blast::ExtPxSubchunk> subchunks = {})
            : m_desc(desc)
            , m_chunks(chunks)
            , m_subchunks(subchunks)
        {
        }

        NvBlastActorDesc& getDefaultActorDesc() override
        {
            return m_desc;
        }

        uint32_t getChunkCount() const override
        {
            return static_cast<uint32_t>(m_chunks.size());
        }

        const Nv::Blast::ExtPxChunk* getChunks() const override
        {
            return m_chunks.data();
        }

        uint32_t getSubchunkCount() const override
        {
            return static_cast<uint32_t>(m_subchunks.size());
        }

        const Nv::Blast::ExtPxSubchunk* getSubchunks() const override
        {
            return m_subchunks.data();
        }

        const NvBlastActorDesc& getDefaultActorDesc() const override
        {
            return m_desc;
        }

        MOCK_METHOD0(release, void());
        MOCK_CONST_METHOD0(getTkAsset, const Nv::Blast::TkAsset&());
        MOCK_METHOD1(setUniformHealth, void(bool));
        MOCK_METHOD1(setAccelerator, void(NvBlastExtDamageAccelerator*));
        MOCK_CONST_METHOD0(getAccelerator, NvBlastExtDamageAccelerator*());

        NvBlastActorDesc m_desc;
        AZStd::vector<Nv::Blast::ExtPxChunk> m_chunks;
        AZStd::vector<Nv::Blast::ExtPxSubchunk> m_subchunks;
    };

    class MockBlastMeshData : public BlastMeshData
    {
    public:
        MOCK_CONST_METHOD1(GetMeshAsset, const AZ::Data::Asset<AZ::RPI::ModelAsset>&(size_t));
        MOCK_CONST_METHOD0(GetMeshAssets, const AZStd::vector<AZ::Data::Asset<AZ::RPI::ModelAsset>>&());
    };

    class MockTkFramework : public Nv::Blast::TkFramework
    {
    public:
        MOCK_METHOD0(release, void());
        MOCK_CONST_METHOD1(getType, const Nv::Blast::TkType*(Nv::Blast::TkTypeIndex::Enum));
        MOCK_CONST_METHOD1(findObjectByID, Nv::Blast::TkIdentifiable*(const NvBlastID&));
        MOCK_CONST_METHOD1(getObjectCount, uint32_t(const Nv::Blast::TkType&));
        MOCK_CONST_METHOD4(
            getObjects, uint32_t(Nv::Blast::TkIdentifiable**, uint32_t, const Nv::Blast::TkType&, uint32_t));
        MOCK_CONST_METHOD6(
            reorderAssetDescChunks, bool(NvBlastChunkDesc*, uint32_t, NvBlastBondDesc*, uint32_t, uint32_t*, bool));
        MOCK_CONST_METHOD2(ensureAssetExactSupportCoverage, bool(NvBlastChunkDesc*, uint32_t));
        MOCK_METHOD1(createAsset, Nv::Blast::TkAsset*(const Nv::Blast::TkAssetDesc&));
        MOCK_METHOD4(
            createAsset, Nv::Blast::TkAsset*(const NvBlastAsset*, Nv::Blast::TkAssetJointDesc*, uint32_t, bool));
        MOCK_METHOD1(createGroup, Nv::Blast::TkGroup*(const Nv::Blast::TkGroupDesc&));
        MOCK_METHOD1(createActor, Nv::Blast::TkActor*(const Nv::Blast::TkActorDesc&));
        MOCK_METHOD1(createJoint, Nv::Blast::TkJoint*(const Nv::Blast::TkJointDesc&));
    };

    class MockTkActor : public Nv::Blast::TkActor
    {
    public:
        MOCK_METHOD0(release, void());
        MOCK_CONST_METHOD0(getActorLL, const NvBlastActor*());
        MOCK_CONST_METHOD0(getFamily, Nv::Blast::TkFamily&());
        MOCK_CONST_METHOD0(getIndex, uint32_t());
        MOCK_CONST_METHOD0(getGroup, Nv::Blast::TkGroup*());
        MOCK_METHOD0(removeFromGroup, Nv::Blast::TkGroup*());
        MOCK_CONST_METHOD0(getAsset, const Nv::Blast::TkAsset*());
        MOCK_CONST_METHOD0(getVisibleChunkCount, uint32_t());
        MOCK_CONST_METHOD2(getVisibleChunkIndices, uint32_t(uint32_t*, uint32_t));
        MOCK_CONST_METHOD0(getGraphNodeCount, uint32_t());
        MOCK_CONST_METHOD2(getGraphNodeIndices, uint32_t(uint32_t*, uint32_t));
        MOCK_CONST_METHOD0(getBondHealths, const float*());
        MOCK_CONST_METHOD0(getSplitMaxActorCount, uint32_t());
        MOCK_CONST_METHOD0(isPending, bool());
        MOCK_METHOD2(damage, void(const NvBlastDamageProgram&, const void*));
        MOCK_CONST_METHOD3(generateFracture, void(NvBlastFractureBuffers*, const NvBlastDamageProgram&, const void*));
        MOCK_METHOD2(applyFracture, void(NvBlastFractureBuffers*, const NvBlastFractureBuffers*));
        MOCK_CONST_METHOD0(getJointCount, uint32_t());
        MOCK_CONST_METHOD2(getJoints, uint32_t(Nv::Blast::TkJoint**, uint32_t));
        MOCK_CONST_METHOD0(isBoundToWorld, bool());
    };

    class MockTkFamily : public Nv::Blast::TkFamily
    {
    public:
        MOCK_METHOD0(release, void());
        MOCK_CONST_METHOD0(getID, const NvBlastID&());
        MOCK_METHOD1(setID, void(const NvBlastID&));
        MOCK_CONST_METHOD0(getType, const Nv::Blast::TkType&());
        MOCK_CONST_METHOD0(getFamilyLL, const NvBlastFamily*());
        MOCK_CONST_METHOD0(getAsset, const Nv::Blast::TkAsset*());
        MOCK_CONST_METHOD0(getActorCount, uint32_t());
        MOCK_CONST_METHOD3(getActors, uint32_t(Nv::Blast::TkActor**, uint32_t, uint32_t));
        MOCK_METHOD1(addListener, void(Nv::Blast::TkEventListener&));
        MOCK_METHOD1(removeListener, void(Nv::Blast::TkEventListener&));
        MOCK_METHOD1(applyFracture, void(const NvBlastFractureBuffers*));
        MOCK_METHOD2(reinitialize, void(const NvBlastFamily*, Nv::Blast::TkGroup*));
    };

    class MockTkAsset : public Nv::Blast::TkAsset
    {
    public:
        MOCK_METHOD0(release, void());
        MOCK_CONST_METHOD0(getID, const NvBlastID&());
        MOCK_METHOD1(setID, void(const NvBlastID&));
        MOCK_CONST_METHOD0(getType, const Nv::Blast::TkType&());
        MOCK_CONST_METHOD0(getAssetLL, const NvBlastAsset*());
        MOCK_CONST_METHOD0(getChunkCount, uint32_t());
        MOCK_CONST_METHOD0(getLeafChunkCount, uint32_t());
        MOCK_CONST_METHOD0(getBondCount, uint32_t());
        MOCK_CONST_METHOD0(getChunks, const NvBlastChunk*());
        MOCK_CONST_METHOD0(getBonds, const NvBlastBond*());
        MOCK_CONST_METHOD0(getGraph, const NvBlastSupportGraph());
        MOCK_CONST_METHOD0(getDataSize, uint32_t());
        MOCK_CONST_METHOD0(getJointDescCount, uint32_t());
        MOCK_CONST_METHOD0(getJointDescs, const Nv::Blast::TkAssetJointDesc*());
    };

    class MockPhysicsSystemRequestsHandler
        : public Physics::SystemRequestBus::Handler
        , public AZ::Interface<Physics::System>::Registrar
    {
    public:
        MockPhysicsSystemRequestsHandler()
        {
            Physics::SystemRequestBus::Handler::BusConnect();
        }
        ~MockPhysicsSystemRequestsHandler()
        {
            Physics::SystemRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD2(
            CreateShape,
            AZStd::shared_ptr<Physics::Shape>(
                const Physics::ColliderConfiguration&, const Physics::ShapeConfiguration&));
        MOCK_METHOD1(ReleaseNativeMeshObject, void(void*));
        MOCK_METHOD1(ReleaseNativeHeightfieldObject, void(void*));
        MOCK_METHOD1(CreateMaterial, AZStd::shared_ptr<Physics::Material>(const Physics::MaterialConfiguration&));
        MOCK_METHOD0(GetDefaultMaterial, AZStd::shared_ptr<Physics::Material>());
        MOCK_METHOD1(
            CreateMaterialsFromLibrary,
            AZStd::vector<AZStd::shared_ptr<Physics::Material>>(const Physics::MaterialSelection&));
        MOCK_METHOD2(
            UpdateMaterialSelection, bool(const Physics::ShapeConfiguration&, Physics::ColliderConfiguration&));
        MOCK_METHOD3(CookConvexMeshToFile, bool(const AZStd::string&, const AZ::Vector3*, AZ::u32));
        MOCK_METHOD3(CookConvexMeshToMemory, bool(const AZ::Vector3*, AZ::u32, AZStd::vector<AZ::u8>&));
        MOCK_METHOD5(
            CookTriangleMeshToFile, bool(const AZStd::string&, const AZ::Vector3*, AZ::u32, const AZ::u32*, AZ ::u32));
        MOCK_METHOD5(
            CookTriangleMeshToMemory,
            bool(const AZ::Vector3*, AZ::u32, const AZ::u32*, AZ::u32, AZStd::vector<AZ::u8>&));
    };

    class MockPhysicsDefaultWorldRequestsHandler : public Physics::DefaultWorldBus::Handler
    {
    public:
        MockPhysicsDefaultWorldRequestsHandler()
        {
            Physics::DefaultWorldBus::Handler::BusConnect();
        }
        ~MockPhysicsDefaultWorldRequestsHandler()
        {
            Physics::DefaultWorldBus::Handler::BusDisconnect();
        }
        MOCK_CONST_METHOD0(GetDefaultSceneHandle, AzPhysics::SceneHandle());
    };

    class MockBlastListener : public BlastListener
    {
    public:
        MOCK_METHOD2(OnActorCreated, void(const BlastFamily&, const BlastActor&));
        MOCK_METHOD2(OnActorDestroyed, void(const BlastFamily&, const BlastActor&));
    };

    class FakeBlastActor : public BlastActor
    {
    public:
        FakeBlastActor(bool isStatic, AzPhysics::SimulatedBody* worldBody, MockTkActor* tkActor)
            : m_isStatic(isStatic)
            , m_transform(worldBody->GetTransform())
            , m_worldBody(worldBody)
            , m_tkActor(tkActor)
        {
        }

        AZ::Transform GetTransform() const override
        {
            return m_transform;
        }

        AzPhysics::SimulatedBody* GetSimulatedBody() override
        {
            return m_worldBody.get();
        }

        const AzPhysics::SimulatedBody* GetSimulatedBody() const override
        {
            return m_worldBody.get();
        }

        const AZ::Entity* GetEntity() const override
        {
            return &m_entity;
        }

        bool IsStatic() const override
        {
            return m_isStatic;
        }

        const AZStd::vector<uint32_t>& GetChunkIndices() const override
        {
            return m_chunkIndices;
        }

        Nv::Blast::TkActor& GetTkActor() const override
        {
            return *m_tkActor;
        }

        MOCK_METHOD2(Damage, void(const NvBlastDamageProgram&, NvBlastExtProgramParams*));
        MOCK_CONST_METHOD0(GetFamily, const BlastFamily&());

        bool m_isStatic;
        AZ::Transform m_transform;
        AZStd::vector<uint32_t> m_chunkIndices;
        AZ::Entity m_entity;
        AZStd::unique_ptr<AzPhysics::SimulatedBody> m_worldBody;
        AZStd::unique_ptr<MockTkActor> m_tkActor;
    };

    class MockShape : public Physics::Shape
    {
    public:
        MOCK_METHOD1(SetMaterial, void(const AZStd::shared_ptr<Physics::Material>&));
        MOCK_CONST_METHOD0(GetMaterial, AZStd::shared_ptr<Physics::Material>());
        MOCK_METHOD1(SetCollisionLayer, void (const AzPhysics::CollisionLayer&));
        MOCK_CONST_METHOD0(GetCollisionLayer, AzPhysics::CollisionLayer ());
        MOCK_METHOD1(SetCollisionGroup, void (const AzPhysics::CollisionGroup&));
        MOCK_CONST_METHOD0(GetCollisionGroup, AzPhysics::CollisionGroup ());
        MOCK_METHOD1(SetName, void(const char*));
        MOCK_METHOD2(SetLocalPose, void(const AZ::Vector3&, const AZ::Quaternion&));
        MOCK_CONST_METHOD0(GetLocalPose, AZStd::pair<AZ::Vector3, AZ::Quaternion>());
        MOCK_CONST_METHOD0(GetRestOffset, float());
        MOCK_CONST_METHOD0(GetContactOffset, float());
        MOCK_METHOD1(SetRestOffset, void(float));
        MOCK_METHOD1(SetContactOffset, void(float));
        MOCK_METHOD0(GetNativePointer, void*());
        MOCK_CONST_METHOD0(GetTag, AZ::Crc32());
        MOCK_METHOD1(AttachedToActor, void(void*));
        MOCK_METHOD0(DetachedFromActor, void());
        MOCK_METHOD2(RayCast, AzPhysics::SceneQueryHit(const AzPhysics::RayCastRequest&, const AZ::Transform&));
        MOCK_METHOD1(RayCastLocal, AzPhysics::SceneQueryHit(const AzPhysics::RayCastRequest&));
        MOCK_CONST_METHOD1(GetAabb, AZ::Aabb(const AZ::Transform&));
        MOCK_CONST_METHOD0(GetAabbLocal, AZ::Aabb());
        MOCK_METHOD3(GetGeometry, void(AZStd::vector<AZ::Vector3>&, AZStd::vector<AZ::u32>&, AZ::Aabb*));
    };

    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")

    class FakeRigidBody : public AzPhysics::RigidBody
    {
    public:
        FakeRigidBody(
            AZ::EntityId entityId = AZ::EntityId(0), AZ::Transform transform = AZ::Transform::CreateIdentity())
            : m_entityId(entityId)
            , m_transform(transform)
        {
        }

        void UpdateMassProperties(
            [[maybe_unused]] AzPhysics::MassComputeFlags flags,
            [[maybe_unused]] const AZ::Vector3& centerOfMassOffsetOverride,
            [[maybe_unused]] const AZ::Matrix3x3& inertiaTensorOverride,
            [[maybe_unused]] const float massOverride) override
        {
        }

        void AddShape(AZStd::shared_ptr<Physics::Shape> shape) override {}

        void RemoveShape(AZStd::shared_ptr<Physics::Shape> shape) override {}

        AZ::Vector3 GetCenterOfMassWorld() const override
        {
            return {};
        }

        AZ::Vector3 GetCenterOfMassLocal() const override
        {
            return {};
        }

        AZ::Matrix3x3 GetInverseInertiaWorld() const override
        {
            return {};
        }

        AZ::Matrix3x3 GetInverseInertiaLocal() const override
        {
            return {};
        }

        float GetMass() const override
        {
            return {};
        }

        float GetInverseMass() const override
        {
            return {};
        }

        void SetMass([[maybe_unused]] float mass) override {}

        void SetCenterOfMassOffset([[maybe_unused]] const AZ::Vector3& comOffset) override {}

        AZ::Vector3 GetLinearVelocity() const override
        {
            return {};
        }

        void SetLinearVelocity([[maybe_unused]] const AZ::Vector3& velocity) override {}

        AZ::Vector3 GetAngularVelocity() const override
        {
            return {};
        }

        void SetAngularVelocity([[maybe_unused]] const AZ::Vector3& angularVelocity) override {}

        AZ::Vector3 GetLinearVelocityAtWorldPoint([[maybe_unused]] const AZ::Vector3& worldPoint) const override
        {
            return {};
        }

        void ApplyLinearImpulse([[maybe_unused]] const AZ::Vector3& impulse) override {}

        void ApplyLinearImpulseAtWorldPoint(
            [[maybe_unused]] const AZ::Vector3& impulse, [[maybe_unused]] const AZ::Vector3& worldPoint) override
        {
        }

        void ApplyAngularImpulse([[maybe_unused]] const AZ::Vector3& angularImpulse) override {}

        float GetLinearDamping() const override
        {
            return {};
        }

        void SetLinearDamping([[maybe_unused]] float damping) override {}

        float GetAngularDamping() const override
        {
            return {};
        }

        void SetAngularDamping([[maybe_unused]] float damping) override {}

        bool IsAwake() const override
        {
            return {};
        }

        void ForceAsleep() override {}

        void ForceAwake() override {}

        float GetSleepThreshold() const override
        {
            return {};
        }

        void SetSleepThreshold([[maybe_unused]] float threshold) override {}

        bool IsKinematic() const override
        {
            return {};
        }

        void SetKinematic([[maybe_unused]] bool kinematic) override {}

        void SetKinematicTarget([[maybe_unused]] const AZ::Transform& targetPosition) override {}

        bool IsGravityEnabled() const override
        {
            return {};
        }

        void SetGravityEnabled([[maybe_unused]] bool enabled) override {}

        void SetSimulationEnabled([[maybe_unused]] bool enabled) override {}

        void SetCCDEnabled([[maybe_unused]] bool enabled) override {}

        AZ::EntityId GetEntityId() const override
        {
            return m_entityId;
        }

        AZ::Transform GetTransform() const override
        {
            return m_transform;
        }

        void SetTransform(const AZ::Transform& transform) override
        {
            m_transform = transform;
        }

        AZ::Vector3 GetPosition() const override
        {
            return m_transform.GetTranslation();
        }

        AZ::Quaternion GetOrientation() const override
        {
            return m_transform.GetRotation();
        }

        AZ::Aabb GetAabb() const override
        {
            return {};
        }

        AzPhysics::SceneQueryHit RayCast([[maybe_unused]] const AzPhysics::RayCastRequest& request) override
        {
            return {};
        }

        AZ::Crc32 GetNativeType() const override
        {
            return NULL;
        }

        void* GetNativePointer() const override
        {
            return nullptr;
        }

        AZ::EntityId m_entityId;
        AZ::Transform m_transform;
    };

    AZ_POP_DISABLE_WARNING

    class FakeActorFactory : public BlastActorFactory
    {
    public:
        MOCK_CONST_METHOD2(
            CalculateVisibleChunks, AZStd::vector<uint32_t>(const BlastFamily&, const Nv::Blast::TkActor&));
        MOCK_CONST_METHOD2(CalculateIsLeafChunk, bool(const Nv::Blast::TkActor&, const AZStd::vector<uint32_t>&));
        MOCK_CONST_METHOD3(
            CalculateIsStatic, bool(const BlastFamily&, const Nv::Blast::TkActor&, const AZStd::vector<uint32_t>&));
        MOCK_CONST_METHOD1(CalculateComponents, AZStd::vector<AZ::Uuid>(bool));

        FakeActorFactory(uint32_t size, bool isStatic = false)
            : m_mockActors(size)
            , m_index(0)
        {
            for (uint32_t i = 0; i < size; ++i)
            {
                m_mockActors[i] = aznew FakeBlastActor(isStatic, aznew FakeRigidBody(), new MockTkActor());
            }
        }

        ~FakeActorFactory()
        {
            for (uint32_t i = 0; i < m_mockActors.size(); ++i)
            {
                delete m_mockActors[i];
            }
            m_mockActors.clear();
        }

        BlastActor* CreateActor([[maybe_unused]] const BlastActorDesc& desc) override
        {
            return m_mockActors[m_index++];
        }

        void DestroyActor([[maybe_unused]] BlastActor* actor) override {}

        std::vector<FakeBlastActor*> m_mockActors;
        int m_index;
    };

    class FakeEntityProvider : public EntityProvider
    {
    public:
        FakeEntityProvider(uint32_t entityCount)
        {
            for (uint32 i = 0; i < entityCount; ++i)
            {
                m_entities.push_back(AZStd::make_shared<AZ::Entity>());
            }
            for (auto& entity : m_entities)
            {
                m_createdEntityIds.push_back(entity->GetId());
            }
        }

        AZStd::shared_ptr<AZ::Entity> CreateEntity([[maybe_unused]] const AZStd::vector<AZ::Uuid>& components) override
        {
            auto entity = m_entities.back();
            m_entities.pop_back();
            return entity;
        }

        AZStd::vector<AZ::EntityId> m_createdEntityIds;
        AZStd::vector<AZStd::shared_ptr<AZ::Entity>> m_entities;
    };

    class MockTransformBusHandler : public AZ::TransformBus::MultiHandler
    {
    public:
        void Connect(AZ::EntityId id)
        {
            AZ::TransformBus::MultiHandler::BusConnect(id);
        }

        ~MockTransformBusHandler()
        {
            AZ::TransformBus::MultiHandler::BusDisconnect();
        }

        MOCK_METHOD1(BindTransformChangedEventHandler, void(AZ::TransformChangedEvent::Handler&));
        MOCK_METHOD1(BindParentChangedEventHandler, void(AZ::ParentChangedEvent::Handler&));
        MOCK_METHOD1(BindChildChangedEventHandler, void(AZ::ChildChangedEvent::Handler&));
        MOCK_METHOD2(NotifyChildChangedEvent, void(AZ::ChildChangeType, AZ::EntityId));
        MOCK_METHOD0(GetLocalTM, const AZ::Transform&());
        MOCK_METHOD1(SetLocalTM, void(const AZ::Transform&));
        MOCK_METHOD0(GetWorldTM, const AZ::Transform&());
        MOCK_METHOD1(SetWorldTM, void(const AZ::Transform&));
        MOCK_METHOD2(GetLocalAndWorld, void(AZ::Transform&, AZ::Transform&));
        MOCK_METHOD1(SetWorldTranslation, void(const AZ::Vector3&));
        MOCK_METHOD1(SetLocalTranslation, void(const AZ::Vector3&));
        MOCK_METHOD0(GetWorldTranslation, AZ::Vector3());
        MOCK_METHOD0(GetLocalTranslation, AZ::Vector3());
        MOCK_METHOD1(MoveEntity, void(const AZ::Vector3&));
        MOCK_METHOD1(SetWorldX, void(float));
        MOCK_METHOD1(SetWorldY, void(float));
        MOCK_METHOD1(SetWorldZ, void(float));
        MOCK_METHOD0(GetWorldX, float());
        MOCK_METHOD0(GetWorldY, float());
        MOCK_METHOD0(GetWorldZ, float());
        MOCK_METHOD1(SetLocalX, void(float));
        MOCK_METHOD1(SetLocalY, void(float));
        MOCK_METHOD1(SetLocalZ, void(float));
        MOCK_METHOD0(GetLocalX, float());
        MOCK_METHOD0(GetLocalY, float());
        MOCK_METHOD0(GetLocalZ, float());
        MOCK_METHOD1(SetWorldRotationQuaternion, void(const AZ::Quaternion&));
        MOCK_METHOD0(GetWorldRotation, AZ::Vector3());
        MOCK_METHOD0(GetWorldRotationQuaternion, AZ::Quaternion());
        MOCK_METHOD1(SetLocalRotation, void(const AZ::Vector3&));
        MOCK_METHOD1(SetLocalRotationQuaternion, void(const AZ::Quaternion&));
        MOCK_METHOD1(RotateAroundLocalX, void(float));
        MOCK_METHOD1(RotateAroundLocalY, void(float));
        MOCK_METHOD1(RotateAroundLocalZ, void(float));
        MOCK_METHOD0(GetLocalRotation, AZ::Vector3());
        MOCK_METHOD0(GetLocalRotationQuaternion, AZ::Quaternion());
        MOCK_METHOD0(GetLocalScale, AZ::Vector3());
        MOCK_METHOD1(SetLocalUniformScale, void(float));
        MOCK_METHOD0(GetLocalUniformScale, float());
        MOCK_METHOD0(GetWorldUniformScale, float());
        MOCK_METHOD0(GetParentId, AZ::EntityId());
        MOCK_METHOD0(GetParent, TransformInterface*());
        MOCK_METHOD1(SetParent, void(AZ::EntityId));
        MOCK_METHOD1(SetParentRelative, void(AZ::EntityId));
        MOCK_METHOD0(GetChildren, AZStd::vector<AZ::EntityId>());
        MOCK_METHOD0(GetAllDescendants, AZStd::vector<AZ::EntityId>());
        MOCK_METHOD0(GetEntityAndAllDescendants, AZStd::vector<AZ::EntityId>());
        MOCK_METHOD0(IsStaticTransform, bool());
        MOCK_METHOD1(SetIsStaticTransform, void(bool));
    };

    class MockRigidBodyRequestBusHandler : public Physics::RigidBodyRequestBus::MultiHandler
    {
    public:
        void Connect(AZ::EntityId id)
        {
            Physics::RigidBodyRequestBus::MultiHandler::BusConnect(id);
        }

        ~MockRigidBodyRequestBusHandler()
        {
            Physics::RigidBodyRequestBus::MultiHandler::BusDisconnect();
        }

        MOCK_METHOD0(EnablePhysics, void());
        MOCK_METHOD0(DisablePhysics, void());
        MOCK_CONST_METHOD0(IsPhysicsEnabled, bool());
        MOCK_CONST_METHOD0(GetCenterOfMassWorld, AZ::Vector3());
        MOCK_CONST_METHOD0(GetCenterOfMassLocal, AZ::Vector3());
        MOCK_CONST_METHOD0(GetInverseInertiaWorld, AZ::Matrix3x3());
        MOCK_CONST_METHOD0(GetInverseInertiaLocal, AZ::Matrix3x3());
        MOCK_CONST_METHOD0(GetMass, float());
        MOCK_CONST_METHOD0(GetInverseMass, float());
        MOCK_METHOD1(SetMass, void(float));
        MOCK_METHOD1(SetCenterOfMassOffset, void(const AZ::Vector3&));
        MOCK_CONST_METHOD0(GetLinearVelocity, AZ::Vector3());
        MOCK_METHOD1(SetLinearVelocity, void(const AZ::Vector3&));
        MOCK_CONST_METHOD0(GetAngularVelocity, AZ::Vector3());
        MOCK_METHOD1(SetAngularVelocity, void(const AZ::Vector3&));
        MOCK_CONST_METHOD1(GetLinearVelocityAtWorldPoint, AZ::Vector3(const AZ::Vector3&));
        MOCK_METHOD1(ApplyLinearImpulse, void(const AZ::Vector3&));
        MOCK_METHOD2(ApplyLinearImpulseAtWorldPoint, void(const AZ::Vector3&, const AZ::Vector3&));
        MOCK_METHOD1(ApplyAngularImpulse, void(const AZ::Vector3&));
        MOCK_CONST_METHOD0(GetLinearDamping, float());
        MOCK_METHOD1(SetLinearDamping, void(float));
        MOCK_CONST_METHOD0(GetAngularDamping, float());
        MOCK_METHOD1(SetAngularDamping, void(float));
        MOCK_CONST_METHOD0(IsAwake, bool());
        MOCK_METHOD0(ForceAsleep, void());
        MOCK_METHOD0(ForceAwake, void());
        MOCK_CONST_METHOD0(GetSleepThreshold, float());
        MOCK_METHOD1(SetSleepThreshold, void(float));
        MOCK_CONST_METHOD0(IsKinematic, bool());
        MOCK_METHOD1(SetKinematic, void(bool));
        MOCK_METHOD1(SetKinematicTarget, void(const AZ::Transform&));
        MOCK_CONST_METHOD0(IsGravityEnabled, bool());
        MOCK_METHOD1(SetGravityEnabled, void(bool));
        MOCK_METHOD1(SetSimulationEnabled, void(bool));
        MOCK_CONST_METHOD0(GetAabb, AZ::Aabb());
        MOCK_METHOD0(GetRigidBody, AzPhysics::RigidBody*());
        MOCK_METHOD1(RayCast, AzPhysics::SceneQueryHit(const AzPhysics::RayCastRequest&));
    };

    class FakeBlastFamily : public BlastFamily
    {
    public:
        FakeBlastFamily()
            : m_pxAsset(NvBlastActorDesc{1, nullptr, 1, nullptr})
        {
        }

        const Nv::Blast::TkFamily* GetTkFamily() const override
        {
            return &m_tkFamily;
        }

        Nv::Blast::TkFamily* GetTkFamily() override
        {
            return &m_tkFamily;
        }

        const Nv::Blast::ExtPxAsset& GetPxAsset() const override
        {
            return m_pxAsset;
        }

        const BlastActorConfiguration& GetActorConfiguration() const override
        {
            return m_actorConfiguration;
        }

        MOCK_METHOD1(Spawn, bool(const AZ::Transform&));
        MOCK_METHOD0(Despawn, void());
        MOCK_METHOD2(HandleEvents, void(const Nv::Blast::TkEvent*, uint32_t));
        MOCK_METHOD1(RegisterListener, void(BlastListener&));
        MOCK_METHOD1(UnregisterListener, void(BlastListener&));
        MOCK_METHOD1(DestroyActor, void(BlastActor*));
        MOCK_METHOD0(GetActorTracker, ActorTracker&());
        MOCK_METHOD3(FillDebugRender, void(DebugRenderBuffer&, DebugRenderMode, float));

        FakeExtPxAsset m_pxAsset;
        MockTkFamily m_tkFamily;
        BlastActorConfiguration m_actorConfiguration;
    };

    class MockBlastSystemBusHandler : public AZ::Interface<Blast::BlastSystemRequests>::Registrar
    {
    public:
        MOCK_CONST_METHOD0(GetTkFramework, Nv::Blast::TkFramework*());
        MOCK_CONST_METHOD0(GetExtSerialization, Nv::Blast::ExtSerialization*());
        MOCK_METHOD0(GetTkGroup, Nv::Blast::TkGroup*());
        MOCK_CONST_METHOD0(GetGlobalConfiguration, const BlastGlobalConfiguration&());
        MOCK_METHOD1(SetGlobalConfiguration, void(const BlastGlobalConfiguration&));
        MOCK_METHOD0(InitPhysics, void());
        MOCK_METHOD0(DeactivatePhysics, void());
        MOCK_METHOD1(AddDamageDesc, void(AZStd::unique_ptr<NvBlastExtRadialDamageDesc>));
        MOCK_METHOD1(AddDamageDesc, void(AZStd::unique_ptr<NvBlastExtCapsuleRadialDamageDesc>));
        MOCK_METHOD1(AddDamageDesc, void(AZStd::unique_ptr<NvBlastExtShearDamageDesc>));
        MOCK_METHOD1(AddDamageDesc, void(AZStd::unique_ptr<NvBlastExtTriangleIntersectionDamageDesc>));
        MOCK_METHOD1(AddDamageDesc, void(AZStd::unique_ptr<NvBlastExtImpactSpreadDamageDesc>));
        MOCK_METHOD1(AddProgramParams, void(AZStd::unique_ptr<NvBlastExtProgramParams>));
        MOCK_METHOD1(SetDebugRenderMode, void(DebugRenderMode));
    };
} // namespace Blast
