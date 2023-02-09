/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/Utils.h>
#include <AZTestShared/Utils/Utils.h>
#include <AzFramework/Physics/PhysicsScene.h>

#include <PhysXCharacters/API/CharacterUtils.h>
#include <PhysXCharacters/Components/RagdollComponent.h>
#include <PhysX/NativeTypeIdentifiers.h>
#include <PhysX/PhysXLocks.h>
#include <Scene/PhysXScene.h>
#include <Tests/PhysXTestFixtures.h>
#include <Tests/PhysXTestCommon.h>
#include <Tests/RagdollTestData.h>
#include <Tests/RagdollConfiguration.h>

namespace PhysX
{
    TEST_F(PhysXDefaultWorldTest, RagdollComponentSerialization_SharedPointerVersion1_NotRegisteredErrorDoesNotOccur)
    {
        // A stream buffer corresponding to a ragdoll component that was serialized before the "PhysXRagdoll" element
        // was changed from a shared pointer to a unique pointer.  Without a valid converter, deserializing this will
        // cause an error.
        const char* objectStreamBuffer =
            R"DELIMITER(<ObjectStream version="1">
            <Class name="RagdollComponent" field="m_template" version="1" type="{B89498F8-4718-42FE-A457-A377DD0D61A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="0" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
                <Class name="AZStd::shared_ptr" field="PhysXRagdoll" type="{A3E470C6-D6E0-5A32-9E83-96C379D9E7FA}"/>
            </Class>
            </ObjectStream>)DELIMITER";

        UnitTest::ErrorHandler errorHandler("not registered with the serializer");
        AZ::Utils::LoadObjectFromBuffer<RagdollComponent>(objectStreamBuffer, strlen(objectStreamBuffer) + 1);

        // Check that there were no errors during deserialization.
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);
    }

    Physics::RagdollState GetTPose(Physics::SimulationType simulationType = Physics::SimulationType::Simulated)
    {
        Physics::RagdollState ragdollState;
        for (int nodeIndex = 0; nodeIndex < RagdollTestData::NumNodes; nodeIndex++)
        {
            Physics::RagdollNodeState nodeState;
            nodeState.m_position = RagdollTestData::NodePositions[nodeIndex];
            nodeState.m_orientation = RagdollTestData::NodeOrientations[nodeIndex];
            nodeState.m_simulationType = simulationType;
            ragdollState.push_back(nodeState);
        }

        return ragdollState;
    }

    Ragdoll* CreateRagdoll(AzPhysics::SceneHandle sceneHandle)
    {
        Physics::RagdollConfiguration* configuration = AZ::Utils::LoadObjectFromBuffer<Physics::RagdollConfiguration>(
            RagdollTestData::RagdollConfiguration.data(), RagdollTestData::RagdollConfiguration.size());
        if (!configuration)
        {
            return nullptr;
        }

        configuration->m_initialState = GetTPose();
        configuration->m_parentIndices.reserve(configuration->m_nodes.size());
        for (int i = 0; i < configuration->m_nodes.size(); i++)
        {
            configuration->m_parentIndices.push_back(RagdollTestData::ParentIndices[i]);
        }

        Ragdoll* ragdoll = nullptr;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SimulatedBodyHandle bodyHandle = sceneInterface->AddSimulatedBody(sceneHandle, configuration);
            ragdoll = azdynamic_cast<Ragdoll*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, bodyHandle));
        }

        delete configuration;
        return ragdoll;
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_GetNativeType_CorrectType)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);
        EXPECT_EQ(ragdoll->GetNativeType(), NativeTypeIdentifiers::Ragdoll);

        const size_t numNodes = ragdoll->GetNumNodes();
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            EXPECT_EQ(ragdoll->GetNode(nodeIndex)->GetNativeType(), NativeTypeIdentifiers::RagdollNode);
        }
    }

    TEST_F(PhysXDefaultWorldTest, RagdollNode_GetNativePointer_CorrectType)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);

        const size_t numNodes = ragdoll->GetNumNodes();
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            auto nativePointer = static_cast<physx::PxBase*>(ragdoll->GetNode(nodeIndex)->GetNativePointer());
            EXPECT_EQ(nativePointer->getConcreteType(), physx::PxConcreteType::eRIGID_DYNAMIC);
        }
    }

    TEST_F(PhysXDefaultWorldTest, RagdollNode_GetTransform_MatchesTestSetup)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);
        ragdoll->EnableSimulation(GetTPose());

        for (size_t nodeIndex = 0; nodeIndex < RagdollTestData::NumNodes; nodeIndex++)
        {
            auto orientation = ragdoll->GetNode(nodeIndex)->GetOrientation();
            auto position = ragdoll->GetNode(nodeIndex)->GetPosition();
            auto transform = ragdoll->GetNode(nodeIndex)->GetTransform();
            EXPECT_TRUE(orientation.IsClose(RagdollTestData::NodeOrientations[nodeIndex]));
            EXPECT_TRUE(position.IsClose(RagdollTestData::NodePositions[nodeIndex]));
            EXPECT_TRUE(transform.IsClose(AZ::Transform::CreateFromQuaternionAndTranslation(
                RagdollTestData::NodeOrientations[nodeIndex], RagdollTestData::NodePositions[nodeIndex])));
        }
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_GetTransform_MatchesTestSetup)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);

        auto orientation = ragdoll->GetOrientation();
        auto position = ragdoll->GetPosition();
        auto transform = ragdoll->GetTransform();
        EXPECT_TRUE(orientation.IsClose(RagdollTestData::NodeOrientations[0]));
        EXPECT_TRUE(position.IsClose(RagdollTestData::NodePositions[0]));
        EXPECT_TRUE(transform.IsClose(AZ::Transform::CreateFromQuaternionAndTranslation(
            RagdollTestData::NodeOrientations[0], RagdollTestData::NodePositions[0])));
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_GetWorld_CorrectWorld)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);

        // the ragdoll isn't enabled yet, so it shouldn't be in a world
        EXPECT_FALSE(ragdoll->IsSimulated());
        const size_t numNodes = ragdoll->GetNumNodes();
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            EXPECT_FALSE(ragdoll->GetNode(nodeIndex)->IsSimulating());
        }

        ragdoll->EnableSimulation(GetTPose());
        EXPECT_TRUE(ragdoll->IsSimulated());
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            EXPECT_TRUE(ragdoll->GetNode(nodeIndex)->IsSimulating());
        }
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_GetNumNodes_EqualsNumInTestPose)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);
        EXPECT_EQ(ragdoll->GetNumNodes(), RagdollTestData::NumNodes);
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_GetJoint_MatchesTestDataJointStructure)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);
        const size_t numNodes = RagdollTestData::NumNodes;
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            auto node = ragdoll->GetNode(nodeIndex);
            auto joint = node->GetJoint();

            size_t parentIndex = RagdollTestData::ParentIndices[nodeIndex];
            if (parentIndex >= numNodes)
            {
                // the root node shouldn't have a parent or a joint
                EXPECT_EQ(joint, nullptr);
            }
            else
            {
                EXPECT_EQ(joint->GetChildBodyHandle(), node->GetRigidBody().m_bodyHandle);
                EXPECT_EQ(joint->GetParentBodyHandle(), ragdoll->GetNode(parentIndex)->GetRigidBody().m_bodyHandle);
            }
        }
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_GetAabb_MatchesTestPoseAabb)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);
        auto aabb = ragdoll->GetAabb();
        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-0.623f, -0.145f, -0.005f), 1e-3f));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(0.623f, 0.166f, 1.724f), 1e-3f));
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_GetNodeOutsideRange_GeneratesError)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);
        UnitTest::ErrorHandler errorHandler("Invalid node index");

        // this node index should be valid
        ragdoll->GetNode(RagdollTestData::NumNodes - 1);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        // this node index should be out of range
        ragdoll->GetNode(RagdollTestData::NumNodes);
        EXPECT_EQ(errorHandler.GetErrorCount(), 1);
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_GetNodeStateOutsideRange_GeneratesError)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);
        UnitTest::ErrorHandler errorHandler("Invalid node index");

        // this node index should be valid
        Physics::RagdollNodeState nodeState;
        ragdoll->GetNodeState(RagdollTestData::NumNodes - 1, nodeState);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        // this node index should be out of range
        ragdoll->GetNodeState(RagdollTestData::NumNodes, nodeState);
        EXPECT_EQ(errorHandler.GetErrorCount(), 1);
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_SetNodeStateOutsideRange_GeneratesError)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);
        UnitTest::ErrorHandler errorHandler("Invalid node index");

        auto ragdollState = GetTPose();
        auto& nodeState = ragdollState.back();

        // this node index should be valid
        ragdoll->SetNodeState(RagdollTestData::NumNodes - 1, nodeState);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        // this node index should be out of range
        ragdoll->SetNodeState(RagdollTestData::NumNodes, nodeState);
        EXPECT_EQ(errorHandler.GetErrorCount(), 1);
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_SimulateWithKinematicState_AabbDoesNotChange)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);
        auto initialAabb = ragdoll->GetAabb();
        auto kinematicTPose = GetTPose(Physics::SimulationType::Kinematic);

        ragdoll->EnableSimulation(kinematicTPose);
        ragdoll->SetState(kinematicTPose);

        for (int timeStep = 0; timeStep < 10; timeStep++)
        {
            m_defaultScene->StartSimulation(AzPhysics::SystemConfiguration::DefaultFixedTimestep);
            m_defaultScene->FinishSimulation();
            EXPECT_TRUE(ragdoll->GetAabb().GetMax().IsClose(initialAabb.GetMax()));
            EXPECT_TRUE(ragdoll->GetAabb().GetMin().IsClose(initialAabb.GetMin()));
        }
        static_cast<PhysX::PhysXScene*>(m_defaultScene)->FlushTransformSync();
    }

    AZ::u32 GetNumRigidDynamicActors(physx::PxScene* scene)
    {
        PHYSX_SCENE_READ_LOCK(scene);
        return scene->getNbActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC);
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_EnableDisableSimulation_NumActorsInSceneCorrect)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);

        auto pxScene = static_cast<physx::PxScene*>(m_defaultScene->GetNativePointer());
        EXPECT_EQ(GetNumRigidDynamicActors(pxScene), 0);
        EXPECT_FALSE(ragdoll->IsSimulated());

        ragdoll->EnableSimulation(GetTPose());
        EXPECT_EQ(GetNumRigidDynamicActors(pxScene), RagdollTestData::NumNodes);
        EXPECT_TRUE(ragdoll->IsSimulated());

        ragdoll->DisableSimulation();
        EXPECT_EQ(GetNumRigidDynamicActors(pxScene), 0);
        EXPECT_FALSE(ragdoll->IsSimulated());
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_NoOtherGeometry_FallsUnderGravity)
    {
        auto ragdoll = CreateRagdoll(m_testSceneHandle);

        ragdoll->EnableSimulation(GetTPose());

        float z = ragdoll->GetPosition().GetZ();
        float expectedInitialZ = RagdollTestData::NodePositions[0].GetZ();
        EXPECT_NEAR(z, expectedInitialZ, 0.01f);

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 60);

        // after falling for 1 second, should have fallen about 1 / 2 * 9.8 * 1 * 1 = 4.9m
        // but allow plenty of leeway for effects of ragdoll pose changing, damping etc.
        z = ragdoll->GetPosition().GetZ();
        EXPECT_NEAR(z, expectedInitialZ - 4.9f, 0.5f);
    }

    TEST_F(PhysXDefaultWorldTest, Ragdoll_AboveStaticFloor_SettlesOnFloor)
    {
        AZ::Transform floorTransform = AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisZ(-0.5f));
        PhysX::TestUtils::AddStaticFloorToScene(m_testSceneHandle, floorTransform);
        auto ragdoll = CreateRagdoll(m_testSceneHandle);
        ragdoll->EnableSimulation(GetTPose());

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 500);

        // the AABB min z should be close to 0
        // allow a little leeway because there might be a little ground penetration
        float minZ = ragdoll->GetAabb().GetMin().GetZ();
        EXPECT_NEAR(minZ, 0.0f, 0.05f);
    }

    TEST(ComputeHierarchyDepthsTest, DepthValuesCorrect)
    {
        AZStd::vector<size_t> parentIndices =
            { 3, 5, AZStd::numeric_limits<size_t>::max(), 1, 2, 9, 7, 4, 0, 6, 11, 12, 5, 14, 15, 16, 5, 18, 19, 4, 21, 22, 4 };

        const AZStd::vector<Utils::Characters::DepthData> nodeDepths = Utils::Characters::ComputeHierarchyDepths(parentIndices);

        std::vector<int> expectedDepths = { 8, 6, 0, 7, 1, 5, 3, 2, 9, 4, 8, 7, 6, 9, 8, 7, 6, 4, 3, 2, 4, 3, 2 };

        for (size_t i = 0; i < parentIndices.size(); i++)
        {
            EXPECT_EQ(nodeDepths[i].m_depth, expectedDepths[i]);
        }
    }
} // namespace PhysX
