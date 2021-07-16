/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Tests/Mocks/RuntimeRequestsMock.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/SlotNames.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Grammar/PrimitivesExecution.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/Start.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    namespace AbstractCodeModelUnitTestStructures
    {
        class TestMethod : public Nodes::Core::Method
        {
        public:
            void SetupMocks(RuntimeRequestsMock* runtimeRequestsMock)
            {
                SetRuntimeBus(runtimeRequestsMock);
            }
        };

        class TestNode : public Node
        {
        public:
            void SetupMocks(RuntimeRequestsMock* runtimeRequestsMock)
            {
                SetRuntimeBus(runtimeRequestsMock);
            }
        };

        class TestAbstractCodeModel : public Grammar::AbstractCodeModel
        {
        public:
            TestAbstractCodeModel() {};
        };

        void PrepareNodeAndOutOfDateMethod(
            Grammar::ExecutionTreePtr executionTreePtr,
            AbstractCodeModelUnitTestStructures::TestNode* node,
            AbstractCodeModelUnitTestStructures::TestMethod* method,
            AZStd::unordered_multimap<Endpoint, Endpoint>* graphEndpointMap,
            RuntimeRequestsMock* runtimeRequestsMock)
        {
            using ::testing::_;
            using ::testing::Return;

            // Node -> Method -> Empty
            node->SetupMocks(runtimeRequestsMock);
            node->AddSlot(ExecutionSlotConfiguration("ToMethod", ConnectionType::Output));
            executionTreePtr->SetId({ node, node->GetSlotByName("ToMethod") });
            executionTreePtr->MarkInputOutputPreprocessed();

            method->SetupMocks(runtimeRequestsMock);
            method->AddSlot(ExecutionSlotConfiguration("ToNothing", ConnectionType::Output));
            EXPECT_CALL(*runtimeRequestsMock, FindNode(_)).Times(1).WillOnce(Return(method));
            EXPECT_CALL(*runtimeRequestsMock, GetConnectedEndpointIterators(_))
                .Times(2)
                .WillOnce(Return(graphEndpointMap->equal_range(Endpoint())))
                .WillOnce(Return(AZStd::pair< EndpointMapConstIterator, EndpointMapConstIterator >(graphEndpointMap->end(), graphEndpointMap->end())));
        }
    };

    class ScriptCanvasAbstractCodeModelUnitTestFixture
        : public ScriptCanvasUnitTestFixture
    {
    protected:
        RuntimeRequestsMock* m_runtimeRequestsMock;
        AbstractCodeModelUnitTestStructures::TestAbstractCodeModel* m_testAbstractCodeModel;

        void SetUp() override
        {
            ScriptCanvasUnitTestFixture::SetUp();

            m_testAbstractCodeModel = new AbstractCodeModelUnitTestStructures::TestAbstractCodeModel();
            m_runtimeRequestsMock = new RuntimeRequestsMock();
        };

        void TearDown() override
        {
            delete m_testAbstractCodeModel;
            delete m_runtimeRequestsMock;

            ScriptCanvasUnitTestFixture::TearDown();
        };
    };

    /*

    TEST_F(ScriptCanvasAbstractCodeModelUnitTestFixture, Parse_NodeIsDisabled_ReturnFalse)
    {
        using ::testing::Return;

        Node testNode;
        testNode.SetNodeEnabled(false);

        bool actualResult = m_testAbstractCodeModel->Parse(testNode);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasAbstractCodeModelUnitTestFixture, Parse_StartNodeIsEnabled_ReturnTrue)
    {
        Nodes::Core::Start startNode;
        Node* testNode = &startNode;

        bool actualResult = m_testAbstractCodeModel->Parse(*testNode);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasAbstractCodeModelUnitTestFixture, ParseExecutionFunction_NodeConnectedWithOutOfDateMethod_ModelIsNotErrorFree)
    {
        Grammar::ExecutionTreePtr testNodeExecutionTreePtr = AZStd::make_shared<Grammar::ExecutionTree>();
        AbstractCodeModelUnitTestStructures::TestNode testNode;
        AbstractCodeModelUnitTestStructures::TestMethod testMethod;
        AZStd::unordered_multimap<Endpoint, Endpoint> testGraphEndpointMap({ {Endpoint(), Endpoint()} });
        AbstractCodeModelUnitTestStructures::PrepareNodeAndOutOfDateMethod(testNodeExecutionTreePtr, &testNode, &testMethod, &testGraphEndpointMap, m_runtimeRequestsMock);

        m_testAbstractCodeModel->ParseExecutionFunction(testNodeExecutionTreePtr, Slot(ExecutionSlotConfiguration("ToTestNode", ConnectionType::Output)));
        EXPECT_FALSE(m_testAbstractCodeModel->IsErrorFree());
        EXPECT_TRUE(m_testAbstractCodeModel->m_validationEvents.size() == 1);

        testNodeExecutionTreePtr->Clear();
    }

    TEST_F(ScriptCanvasAbstractCodeModelUnitTestFixture, CreateOutput_OutputSlotHasNoCorrespondingDatum_OutputUsesCoppiedDatum)
    {
        AZStd::string expectedSlotName = "TestSlot";
        AbstractCodeModelUnitTestStructures::TestNode testNode;
        testNode.SetupMocks(m_runtimeRequestsMock);
        testNode.AddSlot(DataSlotConfiguration(Data::Type::Boolean(), expectedSlotName, ConnectionType::Output));
        Grammar::ExecutionTreePtr testExecutionTreePtr = AZStd::make_shared<Grammar::ExecutionTree>();
        testExecutionTreePtr->SetId({ &testNode, nullptr });
        testExecutionTreePtr->SetScope(AZStd::make_shared<Grammar::Scope>());

        auto outputAssignmentPtr = m_testAbstractCodeModel->CreateOutput(testExecutionTreePtr, *testNode.GetSlotByName(expectedSlotName), "", "input");
        EXPECT_EQ(Data::Type::Boolean(), outputAssignmentPtr->m_source->m_datum.GetType());
        EXPECT_NE(expectedSlotName, outputAssignmentPtr->m_source->m_datum.GetLabel());
    }

    TEST_F(ScriptCanvasAbstractCodeModelUnitTestFixture, CreateOutput_OutputSlotHasCorrespondingDatum_OutputUsesExistingDatum)
    {
        AZStd::string expectedSlotName = "TestSlot";
        AbstractCodeModelUnitTestStructures::TestNode testNode;
        testNode.SetupMocks(m_runtimeRequestsMock);
        testNode.AddSlot(DataSlotConfiguration(Data::Type::Boolean(), expectedSlotName, ConnectionType::Input));
        Grammar::ExecutionTreePtr testExecutionTreePtr = AZStd::make_shared<Grammar::ExecutionTree>();
        testExecutionTreePtr->SetId({ &testNode, nullptr });
        testExecutionTreePtr->SetScope(AZStd::make_shared<Grammar::Scope>());

        auto outputAssignmentPtr = m_testAbstractCodeModel->CreateOutput(testExecutionTreePtr, *testNode.GetSlotByName(expectedSlotName), "", "return");
        EXPECT_EQ(Data::Type::Boolean(), outputAssignmentPtr->m_source->m_datum.GetType());
        EXPECT_EQ(expectedSlotName, outputAssignmentPtr->m_source->m_datum.GetLabel());
    }
    */
}
