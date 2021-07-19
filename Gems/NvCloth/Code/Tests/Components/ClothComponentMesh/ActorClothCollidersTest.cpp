/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>

#include <Components/ClothComponentMesh/ActorClothColliders.h>

#include <UnitTestHelper.h>
#include <ActorHelper.h>
#include <Integration/Components/ActorComponent.h>

namespace NvCloth
{
    namespace Internal
    {
        extern const size_t NvClothMaxNumSphereColliders;
        extern const size_t NvClothMaxNumCapsuleColliders;
    }
}

namespace UnitTest
{
    //! Fixture to setup entity with actor component.
    class NvClothActorClothColliders
        : public ::testing::Test
    {
    protected:
        // ::testing::Test overrides ...
        void SetUp() override;
        void TearDown() override;

        EMotionFX::Integration::ActorComponent* m_actorComponent = nullptr;

    private:
        AZStd::unique_ptr<AZ::Entity> m_entity;
    };

    void NvClothActorClothColliders::SetUp()
    {
        m_entity = AZStd::make_unique<AZ::Entity>();
        m_entity->CreateComponent<AzFramework::TransformComponent>();
        m_actorComponent = m_entity->CreateComponent<EMotionFX::Integration::ActorComponent>();
        m_entity->Init();
        m_entity->Activate();
    }

    void NvClothActorClothColliders::TearDown()
    {
        m_entity->Deactivate();
        m_actorComponent = nullptr;
        m_entity.reset();
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_DefaultConstruct_ReturnsEmptyData)
    {
        AZ::EntityId entityId;
        NvCloth::ActorClothColliders actorClothColliders(entityId);

        EXPECT_TRUE(actorClothColliders.GetSphereColliders().empty());
        EXPECT_TRUE(actorClothColliders.GetSpheres().empty());
        EXPECT_TRUE(actorClothColliders.GetCapsuleColliders().empty());
        EXPECT_TRUE(actorClothColliders.GetCapsuleIndices().empty());
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_CreateWithInvalidEntityId_ReturnsNull)
    {
        AZ::EntityId entityId;
        AZStd::unique_ptr<NvCloth::ActorClothColliders> actorClothColliders = NvCloth::ActorClothColliders::Create(entityId);

        EXPECT_TRUE(actorClothColliders.get() == nullptr);
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_CreateWithActorWithNoClothColliders_ReturnsNull)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothColliders> actorClothColliders = NvCloth::ActorClothColliders::Create(m_actorComponent->GetEntityId());

        EXPECT_TRUE(actorClothColliders.get() == nullptr);
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_CreateWithActorWithBoxClothCollider_ReturnsNull)
    {
        const char* const jointRootName = "joint_root";

        {
            const auto collider = CreateBoxCollider(jointRootName, AZ::Vector3(0.2f, 0.3f, 0.47f));

            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(jointRootName);
            actor->AddClothCollider(collider);
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        // ActorClothColliders only supports spheres or capsules, other shapes will not be taken into account.
        // Since there is no colliders added then it'll return null.
        AZStd::unique_ptr<NvCloth::ActorClothColliders> actorClothColliders = NvCloth::ActorClothColliders::Create(m_actorComponent->GetEntityId());

        EXPECT_TRUE(actorClothColliders.get() == nullptr);
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_CreateWithActorWithSphereClothCollider_ReturnsValidConstraints)
    {
        const char* const jointRootName = "joint_root";
        const float radius = 2.3f;
        const AZ::Transform colliderOffet = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(65.0f)), AZ::Vector3(-0.5f, 3.0f, 6.0f));
        const AZ::Transform jointTransform = AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, 53.0f, -65.0f));

        {
            const auto collider = CreateSphereCollider(jointRootName, radius, colliderOffet);

            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(jointRootName, jointTransform);
            actor->AddClothCollider(collider);
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothColliders> actorClothColliders = NvCloth::ActorClothColliders::Create(m_actorComponent->GetEntityId());

        EXPECT_TRUE(actorClothColliders.get() != nullptr);

        const AZStd::vector<NvCloth::SphereCollider>& sphereColliders = actorClothColliders->GetSphereColliders();
        const AZStd::vector<AZ::Vector4>& nativeSpheres = actorClothColliders->GetSpheres();
        const AZStd::vector<NvCloth::CapsuleCollider>& capsuleColliders = actorClothColliders->GetCapsuleColliders();
        const AZStd::vector<uint32_t>& nativeCapsuleIndices = actorClothColliders->GetCapsuleIndices();

        ASSERT_EQ(sphereColliders.size(), 1);
        ASSERT_EQ(nativeSpheres.size(), 1);
        EXPECT_TRUE(capsuleColliders.empty());
        EXPECT_TRUE(nativeCapsuleIndices.empty());

        EXPECT_NEAR(sphereColliders[0].m_radius, radius, Tolerance);
        EXPECT_EQ(sphereColliders[0].m_nvSphereIndex, 0);
        EXPECT_EQ(sphereColliders[0].m_jointIndex, 0);
        EXPECT_THAT(sphereColliders[0].m_offsetTransform, IsCloseTolerance(colliderOffet, Tolerance));
        EXPECT_THAT(sphereColliders[0].m_currentModelSpaceTransform, IsCloseTolerance(jointTransform * colliderOffet, Tolerance));

        EXPECT_THAT(nativeSpheres[0].GetAsVector3(), IsCloseTolerance((jointTransform * colliderOffet).GetTranslation(), Tolerance));
        EXPECT_NEAR(nativeSpheres[0].GetW(), radius, Tolerance);
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_CreateWithActorWithCapsuleClothCollider_ReturnsValidConstraints)
    {
        const char* const jointRootName = "joint_root";
        const float height = 4.7f;
        const float radius = 1.2f;
        const AZ::Transform colliderOffet = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(65.0f)), AZ::Vector3(-0.5f, 3.0f, 6.0f));
        const AZ::Transform jointTransform = AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, 53.0f, -65.0f));

        {
            const auto collider = CreateCapsuleCollider(jointRootName, height, radius, colliderOffet);

            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(jointRootName, jointTransform);
            actor->AddClothCollider(collider);
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothColliders> actorClothColliders = NvCloth::ActorClothColliders::Create(m_actorComponent->GetEntityId());

        EXPECT_TRUE(actorClothColliders.get() != nullptr);

        const AZStd::vector<NvCloth::SphereCollider>& sphereColliders = actorClothColliders->GetSphereColliders();
        const AZStd::vector<AZ::Vector4>& nativeSpheres = actorClothColliders->GetSpheres();
        const AZStd::vector<NvCloth::CapsuleCollider>& capsuleColliders = actorClothColliders->GetCapsuleColliders();
        const AZStd::vector<uint32_t>& nativeCapsuleIndices = actorClothColliders->GetCapsuleIndices();

        EXPECT_TRUE(sphereColliders.empty());
        ASSERT_EQ(nativeSpheres.size(), 2); // Each capsule produces 2 spheres
        ASSERT_EQ(capsuleColliders.size(), 1);
        ASSERT_EQ(nativeCapsuleIndices.size(), 2); // Each capsule is 2 indices

        EXPECT_NEAR(capsuleColliders[0].m_height, height, Tolerance);
        EXPECT_NEAR(capsuleColliders[0].m_radius, radius, Tolerance);
        EXPECT_EQ(capsuleColliders[0].m_capsuleIndex, 0);
        EXPECT_EQ(capsuleColliders[0].m_sphereAIndex, 0);
        EXPECT_EQ(capsuleColliders[0].m_sphereBIndex, 1);
        EXPECT_EQ(capsuleColliders[0].m_jointIndex, 0);
        EXPECT_THAT(capsuleColliders[0].m_offsetTransform, IsCloseTolerance(colliderOffet, Tolerance));
        EXPECT_THAT(capsuleColliders[0].m_currentModelSpaceTransform, IsCloseTolerance(jointTransform * colliderOffet, Tolerance));

        EXPECT_THAT(nativeSpheres[0], IsCloseTolerance(AZ::Vector4(1.5f, 54.9577f, -58.514f, radius), Tolerance));
        EXPECT_THAT(nativeSpheres[1], IsCloseTolerance(AZ::Vector4(1.5f, 57.0423f, -59.486f, radius), Tolerance));
        EXPECT_NEAR(nativeSpheres[0].GetAsVector3().GetDistance(nativeSpheres[1].GetAsVector3()), height - 2.0f * radius, Tolerance);

        EXPECT_EQ(nativeCapsuleIndices[0], 0);
        EXPECT_EQ(nativeCapsuleIndices[1], 1);
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_CreateWithActorWithSphereAndCapsuleClothColliders_ReturnsValidConstraints)
    {
        const char* const jointRootName = "joint_root";

        {
            const auto sphereCollider = CreateSphereCollider(jointRootName, 0.2f);
            const auto capsuleCollider = CreateCapsuleCollider(jointRootName, 2.0f, 0.75f);

            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(jointRootName);
            actor->AddClothCollider(sphereCollider);
            actor->AddClothCollider(capsuleCollider);
            actor->AddClothCollider(sphereCollider);
            actor->AddClothCollider(sphereCollider);
            actor->AddClothCollider(capsuleCollider);
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothColliders> actorClothColliders = NvCloth::ActorClothColliders::Create(m_actorComponent->GetEntityId());

        const AZStd::vector<NvCloth::SphereCollider>& sphereColliders = actorClothColliders->GetSphereColliders();
        const AZStd::vector<AZ::Vector4>& nativeSpheres = actorClothColliders->GetSpheres();
        const AZStd::vector<NvCloth::CapsuleCollider>& capsuleColliders = actorClothColliders->GetCapsuleColliders();
        const AZStd::vector<uint32_t>& nativeCapsuleIndices = actorClothColliders->GetCapsuleIndices();

        EXPECT_EQ(sphereColliders.size(), 3);
        EXPECT_EQ(nativeSpheres.size(), 3 + 2*2); // 3 spheres + 2 capsules (2 spheres per capsule)
        EXPECT_EQ(capsuleColliders.size(), 2);
        EXPECT_EQ(nativeCapsuleIndices.size(), 2*2); // 2 capsules (2 indices per capsule)
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_CreateWithActorSurpassingMaxNumberOfSpheres_ConstructsUpToMaxNumberOfSpheres)
    {
        const char* const jointRootName = "joint_root";

        {
            const auto sphereCollider = CreateSphereCollider(jointRootName, 0.2f);

            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(jointRootName);
            for (size_t i = 0; i < NvCloth::Internal::NvClothMaxNumSphereColliders * 2; ++i)
            {
                actor->AddClothCollider(sphereCollider);
            }
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothColliders> actorClothColliders = NvCloth::ActorClothColliders::Create(m_actorComponent->GetEntityId());

        const AZStd::vector<NvCloth::SphereCollider>& sphereColliders = actorClothColliders->GetSphereColliders();
        const AZStd::vector<AZ::Vector4>& nativeSpheres = actorClothColliders->GetSpheres();

        EXPECT_EQ(sphereColliders.size(), NvCloth::Internal::NvClothMaxNumSphereColliders);
        EXPECT_EQ(nativeSpheres.size(), NvCloth::Internal::NvClothMaxNumSphereColliders);
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_CreateWithActorSurpassingMaxNumberOfCapsules_ConstructsUpToMaxNumberOfCapsules)
    {
        // Since each capsule has its own unique two spheres, the maximum number of
        // spheres is reached by the time half of maximum number of capsules is reached.
        const size_t maxNumberOfCapsules = NvCloth::Internal::NvClothMaxNumCapsuleColliders / 2;
        const char* const jointRootName = "joint_root";

        {
            const auto capsuleCollider = CreateCapsuleCollider(jointRootName, 2.0f, 0.75f);

            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(jointRootName);
            for (size_t i = 0; i < maxNumberOfCapsules * 2; ++i)
            {
                actor->AddClothCollider(capsuleCollider);
            }
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothColliders> actorClothColliders = NvCloth::ActorClothColliders::Create(m_actorComponent->GetEntityId());

        const AZStd::vector<AZ::Vector4>& nativeSpheres = actorClothColliders->GetSpheres();
        const AZStd::vector<NvCloth::CapsuleCollider>& capsuleColliders = actorClothColliders->GetCapsuleColliders();
        const AZStd::vector<uint32_t>& nativeCapsuleIndices = actorClothColliders->GetCapsuleIndices();

        EXPECT_EQ(nativeSpheres.size(), maxNumberOfCapsules * 2);
        EXPECT_EQ(capsuleColliders.size(), maxNumberOfCapsules);
        EXPECT_EQ(nativeCapsuleIndices.size(), maxNumberOfCapsules * 2);
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_CreateWithActorWithNoSpaceForAnotherCapsule_CapsuleIsNotAdded)
    {
        const char* const jointRootName = "joint_root";

        {
            const auto sphereCollider = CreateSphereCollider(jointRootName, 0.2f);
            const auto capsuleCollider = CreateCapsuleCollider(jointRootName, 2.0f, 0.75f);

            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(jointRootName);
            for (size_t i = 0; i < NvCloth::Internal::NvClothMaxNumSphereColliders - 1; ++i)
            {
                actor->AddClothCollider(sphereCollider);
            }
            actor->AddClothCollider(capsuleCollider); // This last capsule will not fit because it cannot add 2 additional spheres
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothColliders> actorClothColliders = NvCloth::ActorClothColliders::Create(m_actorComponent->GetEntityId());
        
        EXPECT_TRUE(actorClothColliders->GetCapsuleColliders().empty());
        EXPECT_TRUE(actorClothColliders->GetCapsuleIndices().empty());
    }

    TEST_F(NvClothActorClothColliders, ActorClothColliders_Update_ReturnsUpdatedConstraints)
    {
        const char* const jointRootName = "joint_root";
        const char* const jointChildName = "joint_child";
        const float height = 12.3f;
        const float radius = 2.3f;
        const AZ::Transform sphereColliderOffet = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(65.0f)), AZ::Vector3(-0.5f, 3.0f, 6.0f));
        const AZ::Transform capsuleColliderOffet = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(-5.0f)), AZ::Vector3(2.5f, 6.0f, -4.0f));
        const AZ::Transform jointRootTransform = AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, 53.0f, -65.0f));
        const AZ::Transform jointChildTransform = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationY(AZ::DegToRad(36.0f)), AZ::Vector3(3.0f, -2.3f, 16.0f));

        {
            const auto sphereCollider = CreateSphereCollider(jointRootName, radius, sphereColliderOffet);
            const auto capsuleCollider = CreateCapsuleCollider(jointChildName, height, radius, capsuleColliderOffet);

            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(jointRootName, jointRootTransform);
            actor->AddJoint(jointChildName, jointChildTransform, jointRootName);
            actor->AddClothCollider(sphereCollider);
            actor->AddClothCollider(capsuleCollider);
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothColliders> actorClothColliders = NvCloth::ActorClothColliders::Create(m_actorComponent->GetEntityId());

        // Update actor instance's joints transforms
        const AZ::Transform newJointRootTransform = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationZ(AZ::DegToRad(-32.0f)), AZ::Vector3(2.5f, -6.0f, 0.2f));
        const AZ::Transform newJointChildTransform = AZ::Transform::CreateTranslation(AZ::Vector3(-2.0f, 3.0f, 0.0f));
        EMotionFX::Pose* currentPose = m_actorComponent->GetActorInstance()->GetTransformData()->GetCurrentPose();
        currentPose->SetLocalSpaceTransform(0, newJointRootTransform);
        currentPose->SetLocalSpaceTransform(1, newJointChildTransform);

        actorClothColliders->Update();

        const AZStd::vector<NvCloth::SphereCollider>& sphereColliders = actorClothColliders->GetSphereColliders();
        const AZStd::vector<AZ::Vector4>& nativeSpheres = actorClothColliders->GetSpheres();
        const AZStd::vector<NvCloth::CapsuleCollider>& capsuleColliders = actorClothColliders->GetCapsuleColliders();

        EXPECT_THAT(sphereColliders[0].m_offsetTransform, IsCloseTolerance(sphereColliderOffet, Tolerance));
        EXPECT_THAT(sphereColliders[0].m_currentModelSpaceTransform, IsCloseTolerance(newJointRootTransform * sphereColliderOffet, Tolerance));
        EXPECT_THAT(nativeSpheres[0].GetAsVector3(), IsCloseTolerance((newJointRootTransform * sphereColliderOffet).GetTranslation(), Tolerance));

        EXPECT_THAT(capsuleColliders[0].m_offsetTransform, IsCloseTolerance(capsuleColliderOffet, Tolerance));
        EXPECT_THAT(capsuleColliders[0].m_currentModelSpaceTransform, IsCloseTolerance(newJointRootTransform * newJointChildTransform * capsuleColliderOffet, Tolerance));
        EXPECT_THAT(nativeSpheres[1].GetAsVector3(), IsCloseTolerance(AZ::Vector3(7.87111f, 1.65204f, 0.0353498f), Tolerance));
        EXPECT_THAT(nativeSpheres[2].GetAsVector3(), IsCloseTolerance(AZ::Vector3(7.51548f, 1.08291f, -7.63535), Tolerance));
    }
} // namespace UnitTest
