/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>
#include <Common/ShaderAssetTestUtils.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class ShaderResourceGroupGeneralTests
        : public RPITestFixture
    {
    protected:
        RHI::Ptr<RHI::ShaderResourceGroupLayout> m_testSrgLayout;
        Data::Asset<ShaderAsset> m_testShaderAsset;

        RHI::Ptr<RHI::ShaderResourceGroupLayout> CreateTestSrgLayout(const char* nameId)
        {
            RHI::Ptr<RHI::ShaderResourceGroupLayout> srgLayout = RHI::ShaderResourceGroupLayout::Create();

            srgLayout->SetName(Name(nameId));
            srgLayout->SetBindingSlot(0);
            srgLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{ Name{ "MyBufferA" }, RHI::ShaderInputBufferAccess::ReadWrite, RHI::ShaderInputBufferType::Raw, 1, 4, 1, 1});
            srgLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{ Name{ "MyBufferB" }, RHI::ShaderInputBufferAccess::ReadWrite, RHI::ShaderInputBufferType::Raw, 1, 4, 2, 2});
            srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "MyImageA" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 3, 3});
            srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "MyImageB" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 4, 4});
            srgLayout->AddShaderInput(RHI::ShaderInputSamplerDescriptor{ Name{ "MySamplerA" }, 1, 5, 5});
            srgLayout->AddShaderInput(RHI::ShaderInputSamplerDescriptor{ Name{ "MySamplerB" }, 1, 6, 6});
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "MyFloatA" }, 0, 4, 0, 0});
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "MyFloatB" }, 4, 4, 0, 0});
            srgLayout->Finalize();

            return srgLayout;
        }

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_testSrgLayout = CreateTestSrgLayout("TestSrg");
            m_testShaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), m_testSrgLayout);
        }

        void TearDown() override
        {
            m_testSrgLayout = nullptr;
            m_testShaderAsset.Reset();

            RPITestFixture::TearDown();
        }
    };

    TEST_F(ShaderResourceGroupGeneralTests, TestCreate)
    {        
        Data::Instance<ShaderResourceGroup> srgInstance3 = ShaderResourceGroup::Create(m_testShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());
        Data::Instance<ShaderResourceGroup> srgInstance4 = ShaderResourceGroup::Create(m_testShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());

        EXPECT_TRUE(srgInstance3);
        EXPECT_TRUE(srgInstance4);

        // Instances created via Create should be the same object

        EXPECT_NE(srgInstance3, srgInstance4);
    }

    TEST_F(ShaderResourceGroupGeneralTests, TestResourcePool)
    {
        Data::Instance<ShaderResourceGroup> srgA_instance1 = ShaderResourceGroup::Create(m_testShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());
        Data::Instance<ShaderResourceGroup> srgA_instance2 = ShaderResourceGroup::Create(m_testShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());

        auto srgLayoutB = CreateTestSrgLayout("TestSrgB");
        auto shaderAssetB = CreateTestShaderAsset(Uuid::CreateRandom(), srgLayoutB);
        Data::Instance<ShaderResourceGroup> srgB_instance1 = ShaderResourceGroup::Create(shaderAssetB, AZ::RPI::DefaultSupervariantIndex, srgLayoutB->GetName());
        Data::Instance<ShaderResourceGroup> srgB_instance2 = ShaderResourceGroup::Create(shaderAssetB, AZ::RPI::DefaultSupervariantIndex, srgLayoutB->GetName());

        // All instances based on the same asset should have the same pool

        EXPECT_EQ(srgA_instance1->GetRHIShaderResourceGroup()->GetPool(), srgA_instance2->GetRHIShaderResourceGroup()->GetPool());
        EXPECT_EQ(srgB_instance1->GetRHIShaderResourceGroup()->GetPool(), srgB_instance2->GetRHIShaderResourceGroup()->GetPool());

        // Instances based on a different asset should have a different pool

        EXPECT_NE(srgA_instance1->GetRHIShaderResourceGroup()->GetPool(), srgB_instance1->GetRHIShaderResourceGroup()->GetPool());
        EXPECT_NE(srgA_instance1->GetRHIShaderResourceGroup()->GetPool(), srgB_instance2->GetRHIShaderResourceGroup()->GetPool());
    }

    TEST_F(ShaderResourceGroupGeneralTests, TestLayoutWrapperFunctions)
    {
        Data::Instance<ShaderResourceGroup> testSrg = ShaderResourceGroup::Create(m_testShaderAsset, AZ::RPI::DefaultSupervariantIndex, m_testSrgLayout->GetName());

        EXPECT_TRUE(testSrg->GetLayout());
        EXPECT_EQ(testSrg->FindShaderInputBufferIndex(Name{ "MyBufferA" }), testSrg->GetLayout()->FindShaderInputBufferIndex(Name{ "MyBufferA" }));
        EXPECT_EQ(testSrg->FindShaderInputBufferIndex(Name{ "MyBufferB" }), testSrg->GetLayout()->FindShaderInputBufferIndex(Name{ "MyBufferB" }));
        EXPECT_EQ(testSrg->FindShaderInputImageIndex(Name{ "MyImageA" }), testSrg->GetLayout()->FindShaderInputImageIndex(Name{ "MyImageA" }));
        EXPECT_EQ(testSrg->FindShaderInputImageIndex(Name{ "MyImageB" }), testSrg->GetLayout()->FindShaderInputImageIndex(Name{ "MyImageB" }));
        EXPECT_EQ(testSrg->FindShaderInputSamplerIndex(Name{ "MySamplerA" }), testSrg->GetLayout()->FindShaderInputSamplerIndex(Name{ "MySamplerA" }));
        EXPECT_EQ(testSrg->FindShaderInputSamplerIndex(Name{ "MySamplerB" }), testSrg->GetLayout()->FindShaderInputSamplerIndex(Name{ "MySamplerB" }));
        EXPECT_EQ(testSrg->FindShaderInputConstantIndex(Name{ "MyFloatA" }), testSrg->GetLayout()->FindShaderInputConstantIndex(Name{ "MyFloatA" }));
        EXPECT_EQ(testSrg->FindShaderInputConstantIndex(Name{ "MyFloatB" }), testSrg->GetLayout()->FindShaderInputConstantIndex(Name{ "MyFloatB" }));
    }
}
