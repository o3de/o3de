/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAssetCreator.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class ShaderResourceGroupGeneralTests
        : public RPITestFixture
    {
    protected:
        Data::Asset<ShaderResourceGroupAsset> m_testSrgAsset;

        Data::Asset<ShaderResourceGroupAsset> CreateTestSrgAsset(const char* nameId)
        {
            Data::Asset<ShaderResourceGroupAsset> asset;
            ShaderResourceGroupAssetCreator srgCreator;

            srgCreator.Begin(Uuid::CreateRandom(), Name(nameId));
            srgCreator.BeginAPI(RHI::Factory::Get().GetType());
            srgCreator.SetBindingSlot(0);
            srgCreator.AddShaderInput(RHI::ShaderInputBufferDescriptor{ Name{ "MyBufferA" }, RHI::ShaderInputBufferAccess::ReadWrite, RHI::ShaderInputBufferType::Raw, 1, 4, 1 });
            srgCreator.AddShaderInput(RHI::ShaderInputBufferDescriptor{ Name{ "MyBufferB" }, RHI::ShaderInputBufferAccess::ReadWrite, RHI::ShaderInputBufferType::Raw, 1, 4, 2 });
            srgCreator.AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "MyImageA" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 3 });
            srgCreator.AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "MyImageB" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 4 });
            srgCreator.AddShaderInput(RHI::ShaderInputSamplerDescriptor{ Name{ "MySamplerA" }, 1, 5 });
            srgCreator.AddShaderInput(RHI::ShaderInputSamplerDescriptor{ Name{ "MySamplerB" }, 1, 6 });
            srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "MyFloatA" }, 0, 4, 0 });
            srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "MyFloatB" }, 4, 4, 0 });
            srgCreator.EndAPI();
            srgCreator.End(asset);

            return asset;
        }

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_testSrgAsset = CreateTestSrgAsset("TestSrg");
        }

        void TearDown() override
        {
            m_testSrgAsset.Reset();

            RPITestFixture::TearDown();
        }
    };

    TEST_F(ShaderResourceGroupGeneralTests, TestCreateVsFindOrCreate)
    {        
        Data::Instance<ShaderResourceGroup> srgInstance1 = ShaderResourceGroup::FindOrCreate(m_testSrgAsset);
        Data::Instance<ShaderResourceGroup> srgInstance2 = ShaderResourceGroup::FindOrCreate(m_testSrgAsset);
        Data::Instance<ShaderResourceGroup> srgInstance3 = ShaderResourceGroup::Create(m_testSrgAsset);
        Data::Instance<ShaderResourceGroup> srgInstance4 = ShaderResourceGroup::Create(m_testSrgAsset);

        EXPECT_TRUE(srgInstance1);
        EXPECT_TRUE(srgInstance2);
        EXPECT_TRUE(srgInstance3);
        EXPECT_TRUE(srgInstance4);

        // Instances created via FindOrCreate should be the same object

        EXPECT_EQ(srgInstance1, srgInstance2);
        EXPECT_NE(srgInstance1, srgInstance3);
        EXPECT_NE(srgInstance1, srgInstance4);
        EXPECT_NE(srgInstance3, srgInstance4);
    }

    TEST_F(ShaderResourceGroupGeneralTests, TestResourcePool)
    {
        Data::Instance<ShaderResourceGroup> srgA_instance1 = ShaderResourceGroup::Create(m_testSrgAsset);
        Data::Instance<ShaderResourceGroup> srgA_instance2 = ShaderResourceGroup::Create(m_testSrgAsset);

        Data::Asset<ShaderResourceGroupAsset> srgAssetB = CreateTestSrgAsset("TestSrgB");
        Data::Instance<ShaderResourceGroup> srgB_instance1 = ShaderResourceGroup::Create(srgAssetB);
        Data::Instance<ShaderResourceGroup> srgB_instance2 = ShaderResourceGroup::Create(srgAssetB);

        // All instances based on the same asset should have the same pool

        EXPECT_EQ(srgA_instance1->GetRHIShaderResourceGroup()->GetPool(), srgA_instance2->GetRHIShaderResourceGroup()->GetPool());
        EXPECT_EQ(srgB_instance1->GetRHIShaderResourceGroup()->GetPool(), srgB_instance2->GetRHIShaderResourceGroup()->GetPool());

        // Instances based on a different asset should have a different pool

        EXPECT_NE(srgA_instance1->GetRHIShaderResourceGroup()->GetPool(), srgB_instance1->GetRHIShaderResourceGroup()->GetPool());
        EXPECT_NE(srgA_instance1->GetRHIShaderResourceGroup()->GetPool(), srgB_instance2->GetRHIShaderResourceGroup()->GetPool());
    }

    TEST_F(ShaderResourceGroupGeneralTests, TestLayoutWrapperFunctions)
    {
        Data::Instance<ShaderResourceGroup> testSrg = ShaderResourceGroup::Create(m_testSrgAsset);

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
