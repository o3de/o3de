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

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    namespace NodeUnitTestStructures
    {
        class TestNode : public Node
        {
        public:
            void SetupMocks(RuntimeRequestsMock* runtimeRequestsMock)
            {
                SetRuntimeBus(runtimeRequestsMock);

            }
        };
    };

    class ScriptCanvasNodeUnitTestFixture
        : public ScriptCanvasUnitTestFixture
    {
    protected:
        NodeUnitTestStructures::TestNode* m_testNode;
        RuntimeRequestsMock* m_runtimeRequestsMock;

        void SetUp() override
        {
            ScriptCanvasUnitTestFixture::SetUp();

            m_testNode = new NodeUnitTestStructures::TestNode();
            m_runtimeRequestsMock = new RuntimeRequestsMock();
            m_testNode->SetupMocks(m_runtimeRequestsMock);
        };

        void TearDown() override
        {
            delete m_runtimeRequestsMock;
            delete m_testNode;

            ScriptCanvasUnitTestFixture::TearDown();
        };
    };

    TEST_F(ScriptCanvasNodeUnitTestFixture, GetConnectedNodes_NodeIsEnabled_ReturnExpectedNodeWithSlot)
    {
        using ::testing::_;
        using ::testing::Return;

        AZStd::unordered_multimap<Endpoint, Endpoint> testEndpointMap;
        Endpoint expectEndpointOut;
        testEndpointMap.emplace(Endpoint(), expectEndpointOut);
        EXPECT_CALL(*m_runtimeRequestsMock, GetConnectedEndpointIterators(_)).Times(1).WillOnce(Return(testEndpointMap.equal_range(Endpoint())));
        Node expectNode;
        expectNode.SetId(1);
        EXPECT_CALL(*m_runtimeRequestsMock, FindNode(_)).Times(1).WillOnce(Return(&expectNode));
        EndpointsResolved actualNodes = m_testNode->GetConnectedNodes(Slot());
        EXPECT_TRUE(actualNodes.size() == 1);
        EXPECT_TRUE(actualNodes[0].first == &expectNode);
        EXPECT_TRUE(actualNodes[0].second == expectNode.GetSlot(expectEndpointOut.GetSlotId()));
    }

    TEST_F(ScriptCanvasNodeUnitTestFixture, GetConnectedNodes_NodeIsDisabled_ReturnEmpty)
    {
        using ::testing::_;
        using ::testing::Return;

        AZStd::unordered_multimap<Endpoint, Endpoint> testEndpointMap;
        testEndpointMap.emplace(Endpoint(), Endpoint());
        EXPECT_CALL(*m_runtimeRequestsMock, GetConnectedEndpointIterators(_)).Times(1).WillOnce(Return(testEndpointMap.equal_range(Endpoint())));
        Node expectNode;
        expectNode.SetNodeEnabled(false);
        EXPECT_CALL(*m_runtimeRequestsMock, FindNode(_)).Times(1).WillOnce(Return(&expectNode));
        EndpointsResolved actualNodes = m_testNode->GetConnectedNodes(Slot());
        EXPECT_TRUE(actualNodes.size() == 0);
    }
}
