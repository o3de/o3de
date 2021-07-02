/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>
#include <Common/SerializeTester.h>

#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAssetCreator.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderResourceGroupAssetTester
            : public UnitTest::SerializeTester<ShaderResourceGroupAsset>
        {
            using Base = UnitTest::SerializeTester<ShaderResourceGroupAsset>;
        public:
            ShaderResourceGroupAssetTester(AZ::SerializeContext* serializeContext)
                : Base(serializeContext)
            {}

            AZ::Data::Asset<ShaderResourceGroupAsset> SerializeInHelper(const AZ::Data::AssetId& assetId)
            {
                AZ::Data::Asset<ShaderResourceGroupAsset> asset = Base::SerializeIn(assetId);
                asset->FinalizeAfterLoad();
                asset->SetReady();
                return asset;
            }
        };
    }
}

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::RPI;

    class ShaderResourceGroupAssetTests
        : public RPITestFixture
    {
    protected:
        Data::Asset<ShaderResourceGroupAsset> CreateBasicSrgAsset(const Data::AssetId& assetId)
        {
            ShaderResourceGroupAssetCreator creator;
            creator.Begin(assetId, Name("Foo"));
            creator.BeginAPI(RHI::Factory::Get().GetType());

            creator.SetBindingSlot(2);
            creator.AddShaderInput(RHI::ShaderInputBufferDescriptor{ Name{"MyBuffer"}, RHI::ShaderInputBufferAccess::ReadWrite, RHI::ShaderInputBufferType::Raw, 1, 4, 1 });
            creator.AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "MyImage" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 2 });
            creator.AddShaderInput(RHI::ShaderInputSamplerDescriptor{ Name{ "MySampler" }, 1, 3 });
            creator.AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{"MyFloat"}, 0, sizeof(float), 0 });
            creator.AddStaticSampler(RHI::ShaderInputStaticSamplerDescriptor{ Name("MyStaticSampler"), RHI::SamplerState::Create(RHI::FilterMode::Linear, RHI::FilterMode::Point, RHI::AddressMode::Wrap), 4 });

            Data::Asset<ShaderResourceGroupAsset> srgAsset;
            EXPECT_TRUE(creator.EndAPI());
            EXPECT_TRUE(creator.End(srgAsset));

            return srgAsset;
        }
    };

    TEST_F(ShaderResourceGroupAssetTests, Error_Empty)
    {
        AZ_TEST_START_ASSERTTEST;

        ShaderResourceGroupAssetCreator creator;
        creator.Begin(Uuid::CreateRandom(), Name("Foo"));
        creator.BeginAPI(RHI::Factory::Get().GetType());

        Data::Asset<ShaderResourceGroupAsset> srgAsset;
        EXPECT_FALSE(creator.EndAPI());
        EXPECT_FALSE(creator.End(srgAsset));

        AZ_TEST_STOP_ASSERTTEST(2);
    }

    TEST_F(ShaderResourceGroupAssetTests, Basic)
    {
        Data::AssetId assetId(Uuid::CreateRandom());
        Data::Asset<ShaderResourceGroupAsset> srgAsset = CreateBasicSrgAsset(assetId);

        EXPECT_EQ(assetId, srgAsset->GetId());
        EXPECT_EQ(Name("Foo"), srgAsset->GetName());
        EXPECT_EQ(Data::AssetData::AssetStatus::Ready, srgAsset->GetStatus());
        EXPECT_EQ(2, srgAsset->GetLayout()->GetBindingSlot());
        EXPECT_TRUE(srgAsset->GetLayout()->FindShaderInputBufferIndex(Name{ "MyBuffer" }).IsValid());
        EXPECT_TRUE(srgAsset->GetLayout()->FindShaderInputImageIndex(Name{ "MyImage" }).IsValid());
        EXPECT_TRUE(srgAsset->GetLayout()->FindShaderInputSamplerIndex(Name{ "MySampler" }).IsValid());
        EXPECT_TRUE(srgAsset->GetLayout()->FindShaderInputConstantIndex(Name{ "MyFloat" }).IsValid());
        EXPECT_EQ(1, srgAsset->GetLayout()->GetStaticSamplers().size());
    }

    TEST_F(ShaderResourceGroupAssetTests, Serialization)
    {
        Data::AssetId assetId(Uuid::CreateRandom());
        Data::Asset<ShaderResourceGroupAsset> srgAsset = CreateBasicSrgAsset(assetId);

        RPI::ShaderResourceGroupAssetTester tester(GetSerializeContext());
        tester.SerializeOut(srgAsset.Get());
        Data::Asset<RPI::ShaderResourceGroupAsset> serializedSrgAsset = tester.SerializeInHelper(Data::AssetId(Uuid::CreateRandom()));

        EXPECT_EQ(serializedSrgAsset->GetName(), srgAsset->GetName());
        // We don't need to check the entire layout reflection as that should be tested where ShaderResourceGroupLayout is tested.
        EXPECT_EQ(serializedSrgAsset->GetLayout()->GetShaderInput(RHI::ShaderInputBufferIndex  { 0 }).m_name, srgAsset->GetLayout()->GetShaderInput(RHI::ShaderInputBufferIndex  { 0 }).m_name);
        EXPECT_EQ(serializedSrgAsset->GetLayout()->GetShaderInput(RHI::ShaderInputImageIndex   { 0 }).m_name, srgAsset->GetLayout()->GetShaderInput(RHI::ShaderInputImageIndex   { 0 }).m_name);
        EXPECT_EQ(serializedSrgAsset->GetLayout()->GetShaderInput(RHI::ShaderInputSamplerIndex { 0 }).m_name, srgAsset->GetLayout()->GetShaderInput(RHI::ShaderInputSamplerIndex { 0 }).m_name);
        EXPECT_EQ(serializedSrgAsset->GetLayout()->GetShaderInput(RHI::ShaderInputConstantIndex{ 0 }).m_name, srgAsset->GetLayout()->GetShaderInput(RHI::ShaderInputConstantIndex{ 0 }).m_name);
    }

    TEST_F(ShaderResourceGroupAssetTests, Error_NoBegin)
    {
        Data::AssetId assetId(Uuid::CreateRandom());

        AZ_TEST_START_ASSERTTEST;

        ShaderResourceGroupAssetCreator creator;

        creator.SetBindingSlot(2);
        creator.AddShaderInput(RHI::ShaderInputBufferDescriptor{ Name{ "MyBuffer" }, RHI::ShaderInputBufferAccess::ReadWrite, RHI::ShaderInputBufferType::Raw, 1, 4, 1 });
        creator.AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "MyImage" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 2 });
        creator.AddShaderInput(RHI::ShaderInputSamplerDescriptor{ Name{ "MySampler" }, 1, 3 });
        creator.AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "MyFloat" }, 0, sizeof(float), 0 });
        creator.AddStaticSampler(RHI::ShaderInputStaticSamplerDescriptor{ Name("MyStaticSampler"), RHI::SamplerState::Create(RHI::FilterMode::Linear, RHI::FilterMode::Point, RHI::AddressMode::Wrap), 4 });

        Data::Asset<ShaderResourceGroupAsset> srgAsset;
        EXPECT_FALSE(creator.End(srgAsset));

        AZ_TEST_STOP_ASSERTTEST(7);
    }    
}

