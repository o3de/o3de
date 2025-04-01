/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

// Graph Canvas
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/GraphCanvasBus.h>

// GraphModel
#include <GraphModel/GraphModelBus.h>

// GraphModel test harness and minimum setup for a custom GraphModel instance
#include <Tests/TestEnvironment.h>

// These tests rely on this test environment to setup the necessary
// mocked GraphCanvas buses and system components
AZ_UNIT_TEST_HOOK(new GraphModelIntegrationTest::GraphModelTestEnvironment);

namespace GraphModelIntegrationTest
{
    /**
    *   Used for testing the GraphModelIntegration namespace, which relies on GraphCanvasWidgets and Qt.
    *   NOTE: These tests rely on being run in the GraphModelTestEnvironment, which sets up the
    *   necessary mocked GraphCanvas buses and system components.
    */
    class GraphModelIntegrationTests
        : public ::testing::Test
        , public UnitTest::TraceBusRedirector
    {
    protected:
        void SetUp() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();

            // Create our test graph context
            m_graphContext = AZStd::make_shared<TestGraphContext>();

            // Create a new node graph
            m_graph = AZStd::make_shared<GraphModel::Graph>(m_graphContext);

            // Create a new scene for the graph
            AZ::Entity* scene = nullptr;
            GraphModelIntegration::GraphManagerRequestBus::BroadcastResult(scene, &GraphModelIntegration::GraphManagerRequests::CreateScene, m_graph, NODE_GRAPH_TEST_EDITOR_ID);
            m_scene.reset(scene);
            m_sceneId = m_scene->GetId();
        }

        void TearDown() override
        {
            // Release shared pointers that were setup
            m_graphContext.reset();
            m_graph.reset();
            m_scene.reset();

            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        AZStd::shared_ptr<TestGraphContext> m_graphContext = nullptr;
        GraphModel::GraphPtr m_graph = nullptr;
        AZStd::unique_ptr<AZ::Entity> m_scene = nullptr;

        AZ::EntityId m_sceneId;
    };

    TEST_F(GraphModelIntegrationTests, NodeAddedToScene)
    {
        // Create our test node
        GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        GraphCanvas::NodeId nodeId;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, testNode, offset);

        // Make sure the correct node was added to the scene
        GraphModel::NodePtrList nodeList;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeList, m_sceneId, &GraphModelIntegration::GraphControllerRequests::GetNodes);
        EXPECT_EQ(nodeList.size(), 1);
        EXPECT_EQ(nodeList[0], testNode);
    }

    /**
    *   Make sure the data type of the slot on a node is set properly.  There was a new DataSlotConfiguration added
    *   in GraphCanvas, which the GraphModel implementation hadn't previously accounted for, resulting in all data slots on
    *   GraphModel nodes having an invalid data type.
    */
    TEST_F(GraphModelIntegrationTests, NodeWithDataSlotHasProperDataType)
    {
        // Create our test node
        GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        GraphCanvas::NodeId nodeId;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, testNode, offset);

        // Retrieve the data type (string) for the string input slot on our test node
        GraphModel::SlotPtr testNodeStringInputSlot = testNode->GetSlot(TEST_STRING_INPUT_ID);
        EXPECT_TRUE(testNodeStringInputSlot != nullptr);
        GraphModel::DataTypePtr stringDataType = testNodeStringInputSlot->GetDataType();
        AZ::Uuid stringDataTypeId = stringDataType->GetTypeUuid();

        // Make sure our node has the expected slots
        AZStd::vector<AZ::EntityId> slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, nodeId, &GraphCanvas::NodeRequests::GetSlotIds);
        EXPECT_EQ(slotIds.size(), 4);

        // Make sure the data type of the input string slot on our test node matches the expected data type
        AZ::EntityId slotId = slotIds[0];
        AZ::Uuid slotDataTypeId;
        GraphCanvas::DataSlotRequestBus::EventResult(slotDataTypeId, slotId, &GraphCanvas::DataSlotRequests::GetDataTypeId);
        EXPECT_EQ(stringDataTypeId, slotDataTypeId);
    }

    TEST_F(GraphModelIntegrationTests, GetNodeById)
    {
        // Create our test node
        GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        GraphCanvas::NodeId nodeId;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, testNode, offset);

        // Test that we can retrieve the expected node by NodeId
        GraphModel::NodePtr retrievedNode = nullptr;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(retrievedNode, m_sceneId, &GraphModelIntegration::GraphControllerRequests::GetNodeById, nodeId);
        EXPECT_EQ(retrievedNode, testNode);

        // Test requesting an invalid NodeId returns nullptr
        retrievedNode = nullptr;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(retrievedNode, m_sceneId, &GraphModelIntegration::GraphControllerRequests::GetNodeById, GraphCanvas::NodeId());
        EXPECT_EQ(retrievedNode, nullptr);

        // Test requesting a valid NodeId but one that doesn't exist in the scene returns nullptr
        retrievedNode = nullptr;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(retrievedNode, m_sceneId, &GraphModelIntegration::GraphControllerRequests::GetNodeById, GraphCanvas::NodeId(1234));
        EXPECT_EQ(retrievedNode, nullptr);
    }

    TEST_F(GraphModelIntegrationTests, GetNodesFromGraphNodeIds)
    {
        // Create a test node
        GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        GraphCanvas::NodeId nodeId;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, testNode, offset);

        // Retrieve Nodes by their NodeId
        AZStd::vector<GraphCanvas::NodeId> nodeIds = {
            nodeId, // Valid NodeId for a node in the scene
            GraphCanvas::NodeId(), // Invalid NodeId
            GraphCanvas::NodeId(1234) // Valid NodeId but not in the scene
        };
        GraphModel::NodePtrList retrievedNodes;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(retrievedNodes, m_sceneId, &GraphModelIntegration::GraphControllerRequests::GetNodesFromGraphNodeIds, nodeIds);
        // Test that only one node was found.
        EXPECT_EQ(retrievedNodes.size(), 1);

        // Test the first node in the list should be our valid test node
        EXPECT_EQ(retrievedNodes[0], testNode);
    }

    TEST_F(GraphModelIntegrationTests, ExtendableSlotsWithDifferentMinimumValues)
    {
        // Create a node with extendable slots
        GraphModel::NodePtr testNode = AZStd::make_shared<ExtendableSlotsNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        GraphCanvas::NodeId nodeId;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, testNode, offset);

        // The input string extendable slot has a minimum of 0 slots, so there should be none
        auto extendableSlots = testNode->GetExtendableSlots(TEST_STRING_INPUT_ID);
        int numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_INPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 0);
        EXPECT_EQ(numExtendableSlots, 0);

        // The output string and input event extendable slots both use the default minimum (1)
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_OUTPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);
        extendableSlots = testNode->GetExtendableSlots(TEST_EVENT_INPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_EVENT_INPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);

        // The output event extendable slot has a minimum of 3 slots
        extendableSlots = testNode->GetExtendableSlots(TEST_EVENT_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 3);
    }

    TEST_F(GraphModelIntegrationTests, AddingExtendableSlotsPastMaximum)
    {
        // Create a node with extendable slots
        GraphModel::NodePtr testNode = AZStd::make_shared<ExtendableSlotsNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        GraphCanvas::NodeId nodeId;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, testNode, offset);

        // The input string extendable slot has a minimum of 0 slots, so it starts with 0
        auto extendableSlots = testNode->GetExtendableSlots(TEST_STRING_INPUT_ID);
        int numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_INPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 0);
        EXPECT_EQ(numExtendableSlots, 0);

        // The input string extendable slot has a maximum of 2 slots, so the first add should succeed
        GraphModel::SlotPtr firstSlot = testNode->AddExtendedSlot(TEST_STRING_INPUT_ID);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_INPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_INPUT_ID);
        EXPECT_TRUE(firstSlot != nullptr);
        EXPECT_EQ(firstSlot->GetName(), TEST_STRING_INPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);

        // The input string extendable slot has a maximum of 2 slots, so the second add should also succeed
        GraphModel::SlotPtr secondSlot = testNode->AddExtendedSlot(TEST_STRING_INPUT_ID);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_INPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_INPUT_ID);
        EXPECT_TRUE(secondSlot != nullptr);
        EXPECT_EQ(secondSlot->GetName(), TEST_STRING_INPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 2);
        EXPECT_EQ(numExtendableSlots, 2);

        // The input string extendable slot has a maximum of 2 slots, so the third add should fail
        GraphModel::SlotPtr thirdSlot = testNode->AddExtendedSlot(TEST_STRING_INPUT_ID);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_INPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_INPUT_ID);
        EXPECT_TRUE(thirdSlot == nullptr);
        EXPECT_EQ(extendableSlots.size(), 2);
        EXPECT_EQ(numExtendableSlots, 2);
    }

    TEST_F(GraphModelIntegrationTests, RemovingExtendableSlotsBelowMinimum)
    {
        // Create a node with extendable slots
        GraphModel::NodePtr testNode = AZStd::make_shared<ExtendableSlotsNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        GraphCanvas::NodeId nodeId;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, testNode, offset);

        // The output string extendable slot has a minimum of 1 slot, so it starts with 1
        auto extendableSlots = testNode->GetExtendableSlots(TEST_STRING_OUTPUT_ID);
        int numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);

        // The output string extendable slot has the default maximum (100), so we can add one
        GraphModel::SlotPtr firstSlot = testNode->AddExtendedSlot(TEST_STRING_OUTPUT_ID);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_OUTPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_OUTPUT_ID);
        EXPECT_TRUE(firstSlot != nullptr);
        EXPECT_EQ(firstSlot->GetName(), TEST_STRING_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 2);
        EXPECT_EQ(numExtendableSlots, 2);

        // The output string extednable slot has a minimum of 1, so we can remove 1
        testNode->DeleteSlot(firstSlot);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_OUTPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);

        // The output string extednable slot has a minimum of 1, so attempting to remove one
        // when there is only one left will fail
        GraphModel::SlotPtr lastSlot = *extendableSlots.begin();
        testNode->DeleteSlot(lastSlot);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_OUTPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);
    }

    TEST_F(GraphModelIntegrationTests, CannotAddNonExtendableSlot)
    {
        // Create a test node
        GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        GraphCanvas::NodeId nodeId;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, testNode, offset);

        // Can't add a non-extendable slot, so adding a non-extendable slot will fail
        AZ_TEST_START_TRACE_SUPPRESSION;
        GraphModel::SlotPtr newSlot = testNode->AddExtendedSlot(TEST_STRING_INPUT_ID);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(newSlot, nullptr);
    }

    TEST_F(GraphModelIntegrationTests, CannotDeleteNonExtendableSlot)
    {
        // Create a test node
        GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        GraphCanvas::NodeId nodeId;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, testNode, offset);

        // Can't delete a non-extendable slot, so deleting a non-extendable slot will fail
        auto beforeSlots = testNode->GetSlots();
        GraphModel::SlotPtr inputSlot = testNode->GetSlot(TEST_STRING_INPUT_ID);
        testNode->DeleteSlot(inputSlot);
        auto afterSlots = testNode->GetSlots();
        EXPECT_EQ(beforeSlots.size(), afterSlots.size());
    }
}
