/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gmock/gmock.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzTest/AzTest.h>

#include <Blast/BlastSystemBus.h>
#include <Mocks/BlastMocks.h>
#include <Family/DamageManager.h>

using testing::_;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;

namespace Blast
{
    namespace Constants
    {
        static const float DamageAmount = 1.0;
        static const float MinRadius = 0.0;
        static const float MaxRadius = 5.0;
    }

    class DamageManagerTest
        : public testing::Test
        , public FastScopedAllocatorsBase
    {
    public:
    protected:
        void SetUp() override
        {
            m_mockFamily = AZStd::make_shared<FakeBlastFamily>();
            m_actorFactory = AZStd::make_shared<FakeActorFactory>(3);
            m_systemHandler = AZStd::make_shared<MockBlastSystemBusHandler>();
            m_blastMaterial = AZStd::make_shared<Material>(MaterialConfiguration());
            m_damageManager =
                AZStd::make_unique<DamageManager>(m_blastMaterial.get(), m_actorTracker);
        }

        void TearDown() override
        {
            m_mockFamily = nullptr;
            m_actorFactory = nullptr;
            m_systemHandler = nullptr;
            m_damageManager = nullptr;
            m_blastMaterial = nullptr;
        }

        AZStd::unique_ptr<DamageManager> m_damageManager;
        AZStd::shared_ptr<FakeBlastFamily> m_mockFamily;
        AZStd::shared_ptr<FakeActorFactory> m_actorFactory;
        AZStd::shared_ptr<Material> m_blastMaterial;
        AZStd::shared_ptr<MockBlastSystemBusHandler> m_systemHandler;
        ActorTracker m_actorTracker;
    };

    TEST_F(DamageManagerTest, RadialDamage_SUITE_sandbox)
    {
        {
            testing::InSequence dummy;

            EXPECT_CALL(*m_actorFactory->m_mockActors[0], GetFamily()).Times(1).WillOnce(ReturnRef(*m_mockFamily));
            EXPECT_CALL(m_mockFamily->m_pxAsset, getAccelerator()).Times(1).WillOnce(Return(nullptr));
            EXPECT_CALL(*m_actorFactory->m_mockActors[0], Damage(_, _)).Times(1);
            EXPECT_CALL(*m_systemHandler, AddDamageDesc(testing::An<AZStd::unique_ptr<NvBlastExtRadialDamageDesc>>()))
                .Times(1);
            EXPECT_CALL(*m_systemHandler, AddProgramParams(_)).Times(1);
        }

        m_damageManager->Damage(
            DamageManager::RadialDamage{}, *m_actorFactory->m_mockActors[0], Constants::DamageAmount, AZ::Vector3{0, 0, 0}, Constants::MinRadius, Constants::MaxRadius);
    }

    TEST_F(DamageManagerTest, CapsuleDamage_SUITE_sandbox)
    {
        {
            testing::InSequence dummy;

            EXPECT_CALL(*m_actorFactory->m_mockActors[0], GetFamily()).Times(1).WillOnce(ReturnRef(*m_mockFamily));
            EXPECT_CALL(m_mockFamily->m_pxAsset, getAccelerator()).Times(1).WillOnce(Return(nullptr));
            EXPECT_CALL(*m_actorFactory->m_mockActors[0], Damage(_, _)).Times(1);
            EXPECT_CALL(
                *m_systemHandler, AddDamageDesc(testing::An<AZStd::unique_ptr<NvBlastExtCapsuleRadialDamageDesc>>()))
                .Times(1);
            EXPECT_CALL(*m_systemHandler, AddProgramParams(_)).Times(1);
        }

        m_damageManager->Damage(
            DamageManager::CapsuleDamage{}, *m_actorFactory->m_mockActors[0], Constants::DamageAmount,
            AZ::Vector3{0, 0, 0}, AZ::Vector3{1, 0, 0}, Constants::MinRadius, Constants::MaxRadius);
    }

    TEST_F(DamageManagerTest, ShearDamage_SUITE_sandbox)
    {
        {
            testing::InSequence dummy;

            EXPECT_CALL(*m_actorFactory->m_mockActors[0], GetFamily()).Times(1).WillOnce(ReturnRef(*m_mockFamily));
            EXPECT_CALL(m_mockFamily->m_pxAsset, getAccelerator()).Times(1).WillOnce(Return(nullptr));
            EXPECT_CALL(*m_actorFactory->m_mockActors[0], Damage(_, _)).Times(1);
            EXPECT_CALL(*m_systemHandler, AddDamageDesc(testing::An<AZStd::unique_ptr<NvBlastExtShearDamageDesc>>()))
                .Times(1);
            EXPECT_CALL(*m_systemHandler, AddProgramParams(_)).Times(1);
        }

        m_damageManager->Damage(
            DamageManager::ShearDamage{}, *m_actorFactory->m_mockActors[0], Constants::DamageAmount,
            AZ::Vector3{0, 0, 0}, Constants::MinRadius, Constants::MaxRadius, AZ ::Vector3{1, 0, 0});
    }

    TEST_F(DamageManagerTest, TriangleDamage_SUITE_sandbox)
    {
        {
            testing::InSequence dummy;

            EXPECT_CALL(*m_actorFactory->m_mockActors[0], GetFamily()).Times(1).WillOnce(ReturnRef(*m_mockFamily));
            EXPECT_CALL(m_mockFamily->m_pxAsset, getAccelerator()).Times(1).WillOnce(Return(nullptr));
            EXPECT_CALL(*m_actorFactory->m_mockActors[0], Damage(_, _)).Times(1);
            EXPECT_CALL(
                *m_systemHandler,
                AddDamageDesc(testing::An<AZStd::unique_ptr<NvBlastExtTriangleIntersectionDamageDesc>>()))
                .Times(1);
            EXPECT_CALL(*m_systemHandler, AddProgramParams(_)).Times(1);
        }

        m_damageManager->Damage(
            DamageManager::TriangleDamage{}, *m_actorFactory->m_mockActors[0], Constants::DamageAmount,
            AZ::Vector3{0, 0, 0}, AZ::Vector3{1, 0, 0}, AZ::Vector3{0, 1, 0});
    }

    TEST_F(DamageManagerTest, ImpactSpreadDamage_SUITE_sandbox)
    {
        {
            testing::InSequence dummy;

            EXPECT_CALL(*m_actorFactory->m_mockActors[0], GetFamily()).Times(1).WillOnce(ReturnRef(*m_mockFamily));
            EXPECT_CALL(m_mockFamily->m_pxAsset, getAccelerator()).Times(1).WillOnce(Return(nullptr));
            EXPECT_CALL(*m_actorFactory->m_mockActors[0], Damage(_, _)).Times(1);
            EXPECT_CALL(
                *m_systemHandler, AddDamageDesc(testing::An<AZStd::unique_ptr<NvBlastExtImpactSpreadDamageDesc>>()))
                .Times(1);
            EXPECT_CALL(*m_systemHandler, AddProgramParams(_)).Times(1);
        }

        m_damageManager->Damage(
            DamageManager::ImpactSpreadDamage{}, *m_actorFactory->m_mockActors[0], Constants::DamageAmount,
            AZ::Vector3{0, 0, 0}, Constants::MinRadius, Constants::MaxRadius);
    }
} // namespace Blast
