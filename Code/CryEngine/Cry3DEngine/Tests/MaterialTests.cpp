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
#include <Mocks/IRendererMock.h>

#include "Material.h"

class MaterialTest
    : public UnitTest::AllocatorsTestFixture
{
public:
    void SetUp() override
    {
        // capture prior state.
        m_priorEnv = gEnv;
        m_priorSystem = Cry3DEngineBase::m_pSystem;
        m_priorRenderer = Cry3DEngineBase::m_pRenderer;

        UnitTest::AllocatorsTestFixture::SetUp();

        // LegacyAllocator is a lazily-created allocator, so it will always exist, but we still manually shut it down and
        // start it up again between tests so we can have consistent behavior.
        if (!AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
        }

        m_data = AZStd::make_unique<DataMembers>();

        m_data->m_stubEnv.pSystem = &m_data->m_system;
        m_data->m_stubEnv.pRenderer = &m_data->m_renderer;

        // override state
        gEnv = &m_data->m_stubEnv;
        Cry3DEngineBase::m_pSystem = gEnv->pSystem;
        Cry3DEngineBase::m_pRenderer = &m_data->m_renderer;
    }

    void TearDown() override
    {
        m_data.reset();

        AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
        UnitTest::AllocatorsTestFixture::TearDown();

        // restore state.
        gEnv = m_priorEnv;
        Cry3DEngineBase::m_pSystem = m_priorSystem;
        Cry3DEngineBase::m_pRenderer = m_priorRenderer;
    }

    struct DataMembers
    {
        SSystemGlobalEnvironment m_stubEnv;
        ::testing::NiceMock<SystemMock> m_system;
        ::testing::NiceMock<IRendererMock> m_renderer;
    };

    AZStd::unique_ptr<DataMembers> m_data;
    SSystemGlobalEnvironment* m_priorEnv = nullptr;
    ISystem* m_priorSystem = nullptr;
    IRenderer* m_priorRenderer = nullptr;
};

TEST_F(MaterialTest, CMatInfo_SetSubMtl_OutOfRange)
{
    _smart_ptr<IMaterial> materialGroup = new CMatInfo();
    _smart_ptr<IMaterial> validSubMaterial = new CMatInfo();
    _smart_ptr<IMaterial> outOfRangeSubMaterial = new CMatInfo();

    // Make materialGroup into an actual material group
    materialGroup->SetSubMtlCount(1);
    materialGroup->SetSubMtl(0, validSubMaterial);

    AZ_TEST_START_TRACE_SUPPRESSION;
    // SetSubMtl should fail because the index is beyond the range of material's vector of sub-materials
    materialGroup->SetSubMtl(2, outOfRangeSubMaterial);
    materialGroup->SetSubMtl(-1, outOfRangeSubMaterial);
    AZ_TEST_STOP_TRACE_SUPPRESSION(2);

    // Material should still have a 1-size vector of sub-materials, with 'subMaterial' as its only sub-material
    EXPECT_TRUE(materialGroup->IsMaterialGroup());
    EXPECT_EQ(materialGroup->GetSubMtlCount(), 1);
    EXPECT_EQ(materialGroup->GetSubMtl(0), validSubMaterial);
    EXPECT_EQ(materialGroup->GetSubMtl(1), nullptr);
}

TEST_F(MaterialTest, CMatInfo_SetSubMtl_InvalidSubMaterial)
{
    _smart_ptr<IMaterial> materialGroup = new CMatInfo();
    _smart_ptr<IMaterial> inValidSubMaterial = new CMatInfo();
    _smart_ptr<IMaterial> validSubMaterial0 = new CMatInfo();
    _smart_ptr<IMaterial> validSubMaterial1 = new CMatInfo();

    // Make the two materials into material groups by inserting sub-materials
    materialGroup->SetSubMtlCount(2);
    materialGroup->SetSubMtl(0, validSubMaterial0);
    materialGroup->SetSubMtl(1, validSubMaterial1);

    inValidSubMaterial->SetSubMtlCount(2);
    inValidSubMaterial->SetSubMtl(0, validSubMaterial0);
    inValidSubMaterial->SetSubMtl(1, validSubMaterial1);

    // SetSubMtl should fail because subMaterial is a material group, and material groups cannot be sub-materials
    AZ_TEST_START_TRACE_SUPPRESSION;
    materialGroup->SetSubMtl(1, inValidSubMaterial);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);

    // Check that subMaterial is not the material at index 1, since SetSubMtl should have failed
    EXPECT_EQ(materialGroup->GetSubMtl(1), validSubMaterial1);
    EXPECT_NE(materialGroup->GetSubMtl(1), inValidSubMaterial);
}

TEST_F(MaterialTest, CMatInfo_SetSubMtlCount_SetsMaterialGroupFlag)
{
    _smart_ptr<IMaterial> material = new CMatInfo();

    // Check to ensure the material group flag is being set
    material->SetSubMtlCount(1);
    EXPECT_TRUE(material->IsMaterialGroup());

    // Check to ensure the material group flag is being un-set
    material->SetSubMtlCount(0);
    EXPECT_FALSE(material->IsMaterialGroup());
}

TEST_F(MaterialTest, CMatInfo_IsDirty_DoesNotCrash)
{
    // Create a material group with two sub-materials
    _smart_ptr<IMaterial> materialGroup = new CMatInfo();
    materialGroup->SetSubMtlCount(2);

    // Set one sub-material to be null, and the other valid
    _smart_ptr<IMaterial> subMaterial = new CMatInfo();
    materialGroup->SetSubMtl(0, nullptr);
    materialGroup->SetSubMtl(1, subMaterial);

    // Call IsDirty to validate that it does not crash
    EXPECT_FALSE(materialGroup->IsDirty());
}
