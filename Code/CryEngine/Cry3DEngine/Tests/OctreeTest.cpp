/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "Cry3DEngine_precompiled.h"
#include <AzTest/AzTest.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <Mocks/ISystemMock.h>
#include <Mocks/I3DEngineMock.h>
#include "DecalRenderNode.h"

class OctreeTest
    : public UnitTest::AllocatorsTestFixture
{
public:
    void SetUp() override
    {
        // capture prior state.
        m_savedState.m_Env = gEnv;
        m_savedState.m_3DEngine = Cry3DEngineBase::m_p3DEngine;
        m_savedState.m_system = Cry3DEngineBase::m_pSystem;
        m_savedState.m_cvars = Cry3DEngineBase::m_pCVars;

        UnitTest::AllocatorsTestFixture::SetUp();

        // LegacyAllocator is a lazily-created allocator, so it will always exist, but we still manually shut it down and
        // start it up again between tests so we can have consistent behavior.
        if (!AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
        }

        m_data = AZStd::make_unique<DataMembers>();

        // override state with our needed test mocks
        gEnv = &(m_data->m_mockGEnv);
        m_data->m_mockGEnv.p3DEngine = &(m_data->m_mock3DEngine);
        m_data->m_mockGEnv.pSystem = &(m_data->m_system);
        Cry3DEngineBase::m_pSystem = gEnv->pSystem;

        // NOTE:  We've mocked out I3DEngine, but we don't currently have a mock of C3DEngine.
        // For these unit tests, we need all code paths that we're testing to go through I3DEngine.
        // We set the engine's C3DEngine pointer to null to guarantee we aren't using it.
        Cry3DEngineBase::m_p3DEngine = nullptr;

        // Set GetISystem()->GetI3DEngine() to return our mocked I3DEngine.
        EXPECT_CALL(m_data->m_system, GetI3DEngine()).WillRepeatedly(Return(&(m_data->m_mock3DEngine)));

        // Create a default set of CVars for the Octree to read from.
        // This needs to happen *after* we've set gEnv, since CVars()
        // is dependent on it.
        m_mockCVars = new CVars();
        Cry3DEngineBase::m_pCVars = m_mockCVars;

        // Create the test Octree
        const int nSID = 0;
        m_octree = COctreeNode::Create(nSID, AABB(Vec3(0.0f), Vec3(1024.0f)), nullptr);
    }

    void TearDown() override
    {
        // Remove our test octree and mock cvars
        delete m_octree;
        delete m_mockCVars;

        // Remove the other mocked systems
        m_data.reset();

        AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
        UnitTest::AllocatorsTestFixture::TearDown();

        // restore the global state that we modified during the test.
        gEnv = m_savedState.m_Env;
        Cry3DEngineBase::m_p3DEngine = m_savedState.m_3DEngine;
        Cry3DEngineBase::m_pSystem = m_savedState.m_system;
        Cry3DEngineBase::m_pCVars = m_savedState.m_cvars;
    }

    // Helper functions
    IRenderNode* CreateAndAddDecalNode(float radius)
    {
        AABB nodeBox(radius);
        IRenderNode* decalEntity = new CDecalRenderNode();
        decalEntity->SetBBox(nodeBox);
        m_octree->InsertObject(decalEntity, nodeBox, (radius * radius), nodeBox.GetCenter());
        return decalEntity;
    }

    void RemoveNode(IRenderNode* node)
    {
        m_octree->DeleteObject(node);
        delete node;
    }

protected:
    struct DataMembers
    {
        SSystemGlobalEnvironment m_mockGEnv;
        ::testing::NiceMock<I3DEngineMock> m_mock3DEngine;
        ::testing::NiceMock<SystemMock> m_system;
    };

    AZStd::unique_ptr<DataMembers> m_data;

    CVars* m_mockCVars = nullptr;
    COctreeNode* m_octree = nullptr;

private:
    struct SavedState
    {
        SSystemGlobalEnvironment* m_Env = nullptr;
        C3DEngine* m_3DEngine = nullptr;
        ISystem* m_system = nullptr;
        CVars* m_cvars = nullptr;
    };

    SavedState m_savedState;
};
