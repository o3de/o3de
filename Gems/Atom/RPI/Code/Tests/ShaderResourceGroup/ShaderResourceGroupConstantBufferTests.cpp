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
    class ShaderResourceGroupConstantBufferTests
        : public RPITestFixture
    {
    protected:

        struct SimpleStruct
        {
            SimpleStruct() = default;
            SimpleStruct(float f, uint32_t u)
                : m_float{f}
                , m_uint{u}
            {}

            float m_float = 0;
            uint32_t m_uint = 0;
        };
        
        AZ::Data::Asset<AZ::RPI::ShaderAsset> m_shaderAsset;
        AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_srgLayout;
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_srg;

        void SetUp() override
        {
            RPITestFixture::SetUp();

            // This provides the high-level metadata and low-level srg layout
            m_srgLayout = BuildSrgLayoutWithShaderConstants(m_shaderAsset);
            ASSERT_TRUE(m_srgLayout);
            ASSERT_TRUE(m_shaderAsset.IsReady());

            m_srg = AZ::RPI::ShaderResourceGroup::Create(m_shaderAsset, AZ::RPI::DefaultSupervariantIndex, m_srgLayout->GetName());
            ASSERT_TRUE(m_srg != nullptr);
        }

        void TearDown() override
        {
            m_srg.reset();
            m_srgLayout = nullptr;
            m_shaderAsset.Release();

            RPITestFixture::TearDown();
        }

        template<typename T>
        void ExpectEqual(AZStd::initializer_list<T> expectedValues, AZStd::span<const T> arrayView)
        {
            EXPECT_EQ(expectedValues.size(), arrayView.size());

            const T* expected = expectedValues.begin();
            for (int i = 0; i < expectedValues.size() && i < arrayView.size(); ++i)
            {
                EXPECT_EQ(expected[i], arrayView[i]);
            }
        }

        AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> BuildSrgLayoutWithShaderConstants(
            AZ::Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset, [[maybe_unused]] bool includeMetadata = true)
        {
            using namespace AZ;

            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> srgLayout = RHI::ShaderResourceGroupLayout::Create();
            srgLayout->SetName(Name{"TestSrg"});

            uint32_t offset = 0;
            uint32_t count;
            uint32_t size;
            uint32_t registerIndex = 0;
            uint32_t spaceIndex = 0;
            uint32_t sizeOfBool = 4;

            srgLayout->SetBindingSlot(0);

            // bool, binding index 0
            count = 1;
            size = count * sizeOfBool;
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyBool"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // bool2, binding index 1
            count = 2;
            size = count * sizeOfBool;
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyBool2"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // bool3, binding index 2
            count = 3;
            size = count * sizeOfBool;
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyBool3"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // bool4, binding index 3
            count = 4;
            size = count * sizeOfBool;
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyBool4"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // int, binding index 4
            count = 1;
            size = count * sizeof(int32_t);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyInt"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // int2, binding index 5
            count = 2;
            size = count * sizeof(int32_t);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyInt2"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // int3, binding index 6
            count = 3;
            size = count * sizeof(int32_t);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyInt3"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // int4, binding index 7
            count = 4;
            size = count * sizeof(int32_t);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyInt4"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // uint, binding index 8
            count = 1;
            size = count * sizeof(uint32_t);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyUint"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // uint2, binding index 9
            count = 2;
            size = count * sizeof(uint32_t);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyUint2"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // uint3, binding index 10
            count = 3;
            size = count * sizeof(uint32_t);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyUint3"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // uint4, binding index 11
            count = 4;
            size = count * sizeof(uint32_t);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyUint4"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // float, binding index 12
            count = 1;
            size = count * sizeof(float);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyFloat"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // float2, binding index 13
            count = 2;
            size = count * sizeof(float);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyFloat2"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // float3, binding index 14
            count = 3;
            size = count * sizeof(float);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyFloat3"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // float4, binding index 15
            count = 4;
            size = count * sizeof(float);
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MyFloat4"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // simple struct, binding index 16
            // [GFX TODO][ATOM-111] This is not very fleshed out right now. We still need to do more to support structs, but at least I want to verify that SRG templatized setters and getters can work with structs
            count = 1;
            size = 8;
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MySimpleStruct"), offset, size, registerIndex, spaceIndex});
            offset += size;

            // array of 2 simple structs, binding index 17
            // [GFX TODO][ATOM-111] This is not very fleshed out right now. We still need to do more to support structs, but at least I want to verify that SRG templatized setters and getters can work with structs
            count = 2;
            size = 16;
            srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name("MySimpleStructArray2"), offset, size, registerIndex, spaceIndex});
            offset += size;

            srgLayout->SetBindingSlot(0);

            EXPECT_TRUE(srgLayout->Finalize());

            shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), srgLayout);

            return srgLayout;
        }
    };

    TEST_F(ShaderResourceGroupConstantBufferTests, SetConstant_GetConstant_ValidInput_Bool)
    {
        using namespace AZ;

        {
            const RHI::ShaderInputConstantIndex inputIndex(0);

            // Check using inputIndex

            EXPECT_TRUE(m_srg->SetConstant(inputIndex, true));
            EXPECT_EQ(true, m_srg->GetConstant<bool>(inputIndex));
            AZStd::span<const uint8_t> result = m_srg->GetConstantRaw(inputIndex);
            AZStd::span<const uint32_t> resultInUint = AZStd::span<const uint32_t>(reinterpret_cast<const uint32_t*>(result.data()), 1);
            ExpectEqual<uint32_t>({ 1 /*true*/ }, resultInUint);

            EXPECT_TRUE(m_srg->SetConstant(inputIndex, false));
            EXPECT_EQ(false, m_srg->GetConstant<bool>(inputIndex));
            result = m_srg->GetConstantRaw(inputIndex);
            resultInUint = AZStd::span<const uint32_t>(reinterpret_cast<const uint32_t*>(result.data()), 1);
            ExpectEqual<uint32_t>({ 0 /*false*/ }, resultInUint);
        }

        {
            const RHI::ShaderInputConstantIndex inputIndex(1);

            // Check using inputIndex

            EXPECT_TRUE(m_srg->SetConstantArray<bool>(inputIndex, AZStd::array<bool, 2>({ true, false })));
            AZStd::span<const uint8_t> result = m_srg->GetConstantRaw(inputIndex);
            AZStd::span<const uint32_t> resultInUint = AZStd::span<const uint32_t>(reinterpret_cast<const uint32_t*>(result.data()), 2);
            ExpectEqual<uint32_t>({ 1 /*true*/, 0 /*false*/ }, resultInUint);

            EXPECT_TRUE(m_srg->SetConstantArray<bool>(inputIndex, AZStd::array<bool, 2>({ false, true })));
            result = m_srg->GetConstantRaw(inputIndex);
            resultInUint = AZStd::span<const uint32_t>(reinterpret_cast<const uint32_t*>(result.data()), 2);
            ExpectEqual<uint32_t>({ 0 /*false*/, 1 /*true*/ }, resultInUint);
        }
    }
    TEST_F(ShaderResourceGroupConstantBufferTests, SetConstant_GetConstant_FalsePackedInGarbage_Bool)
    {
        using namespace AZ;

        uint32_t falsePackedInGarbage = 0xab00cdef;
        bool* asBools = reinterpret_cast<bool*>(&falsePackedInGarbage);

        {
            const RHI::ShaderInputConstantIndex inputIndex(0);

            // Check using inputIndex

            EXPECT_TRUE(m_srg->SetConstant<bool>(inputIndex, asBools[2]));
            EXPECT_EQ(false, m_srg->GetConstant<bool>(inputIndex));
        }

        {
            // Check using inputIndex

            const RHI::ShaderInputConstantIndex inputIndex(1);
            EXPECT_TRUE(m_srg->SetConstantArray<bool>(inputIndex, AZStd::array<bool, 2>({ asBools[1], asBools[2] })));
            AZStd::span<const uint8_t> result = m_srg->GetConstantRaw(inputIndex);
            AZStd::span<const uint32_t> resultInUint = AZStd::span<const uint32_t>(reinterpret_cast<const uint32_t*>(result.data()), 2);
            EXPECT_THAT(resultInUint, testing::ElementsAre(testing::IsTrue(), testing::IsFalse()));
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Test valid inputs for SetConstant and GetConstant

    TEST_F(ShaderResourceGroupConstantBufferTests, SetConstant_GetConstant_ValidInput_Int)
    {
        using namespace AZ;

        {
            const RHI::ShaderInputConstantIndex inputIndex(4);

            // Check using inputIndex
            EXPECT_TRUE(m_srg->SetConstant(inputIndex, 51));
            EXPECT_EQ(51, m_srg->GetConstant<int32_t>(inputIndex));
            ExpectEqual<int32_t>({ 51 }, m_srg->GetConstantArray<int32_t>(inputIndex));
        }

        {
            const RHI::ShaderInputConstantIndex inputIndex(5);

            // Check using inputIndex
            EXPECT_TRUE(m_srg->SetConstantArray<int32_t>(inputIndex, AZStd::array<int32_t, 2>({ 54, 55 })));
            ExpectEqual<int32_t>({ 54, 55 }, m_srg->GetConstantArray<int32_t>(inputIndex));
        }
    }

    TEST_F(ShaderResourceGroupConstantBufferTests, SetConstant_GetConstant_ValidInput_Float)
    {
        using namespace AZ;

        {
            const RHI::ShaderInputConstantIndex inputIndex(12);

            // Check using inputIndex
            EXPECT_TRUE(m_srg->SetConstant(inputIndex, 1.1f));
            EXPECT_EQ(1.1f, m_srg->GetConstant<float>(inputIndex));
            ExpectEqual<float>({ 1.1f }, m_srg->GetConstantArray<float>(inputIndex));
        }

        {
            const RHI::ShaderInputConstantIndex inputIndex(13);

            // Check using inputIndex
            EXPECT_TRUE(m_srg->SetConstantArray<float>(inputIndex, AZStd::array<float, 2>({ 1.4f, 1.5f })));
            ExpectEqual<float>({ 1.4f, 1.5f }, m_srg->GetConstantArray<float>(inputIndex));
        }
    }

    TEST_F(ShaderResourceGroupConstantBufferTests, SetConstant_GetConstant_ValidInput_Vector4)
    {
        using namespace AZ;

        AZ::Vector4 value;
        const RHI::ShaderInputConstantIndex inputIndex(15);

        // Check using inputIndex

        EXPECT_TRUE(m_srg->SetConstant(inputIndex, AZ::Vector4(2.6f, 2.7f, 2.8f, 2.9f)));
        value = m_srg->GetConstant<AZ::Vector4>(inputIndex);
        EXPECT_EQ(2.6f, static_cast<float>(value.GetX()));
        EXPECT_EQ(2.7f, static_cast<float>(value.GetY()));
        EXPECT_EQ(2.8f, static_cast<float>(value.GetZ()));
        EXPECT_EQ(2.9f, static_cast<float>(value.GetW()));
    }

    TEST_F(ShaderResourceGroupConstantBufferTests, SetConstant_GetConstant_ValidInput_SimpleStruct)
    {
        using namespace AZ;

        SimpleStruct value;
        const RHI::ShaderInputConstantIndex inputIndex(16);

        // Demonstrate the syntax of setting with a variable, and inputIndex
        {
            SimpleStruct inputValues = { 2.1f, 101 };

            EXPECT_TRUE(m_srg->SetConstant(inputIndex, inputValues));
            value = m_srg->GetConstant<SimpleStruct>(inputIndex);

            EXPECT_EQ(2.1f, value.m_float);
            EXPECT_EQ(101, value.m_uint);
        }
    }

    TEST_F(ShaderResourceGroupConstantBufferTests, SetConstant_GetConstant_ValidInput_SimpleStruct_Array)
    {
        using namespace AZ;

        AZStd::span<const SimpleStruct> values;
        const RHI::ShaderInputConstantIndex inputIndex(17);

        // Demonstrate the syntax of setting with a variable, and inputIndex...
        // Unfortunately, with arrays of custom types, you have to specify the element type explicitly
        {
            AZStd::vector<SimpleStruct> inputValues;
            inputValues.push_back({ 0.3f, 3 });
            inputValues.push_back({ 0.4f, 4 });

            EXPECT_TRUE(m_srg->SetConstantArray<SimpleStruct>(inputIndex, inputValues));

            values = m_srg->GetConstantArray<SimpleStruct>(inputIndex);

            EXPECT_EQ(2, values.size());
            EXPECT_EQ(0.3f, values[0].m_float);
            EXPECT_EQ(3, values[0].m_uint);
            EXPECT_EQ(0.4f, values[1].m_float);
            EXPECT_EQ(4, values[1].m_uint);
        }
    }

    TEST_F(ShaderResourceGroupConstantBufferTests, TestErrorReporting_SetConstant_WrongNumberOfElements_ArrayInput)
    {
        using namespace AZ;

        {
            AZ_TEST_START_ASSERTTEST;

            // MyFloat2
            EXPECT_FALSE(m_srg->SetConstantArray<float>(RHI::ShaderInputConstantIndex(13), AZStd::array<float, 3>({ 0.1f, 0.2f, 0.3f })));

            AZ_TEST_STOP_ASSERTTEST(2);
        }
    }

    TEST_F(ShaderResourceGroupConstantBufferTests, TestErrorReporting_GetConstants_WrongNumberOfElements_ArrayOutput)
    {
        using namespace AZ;

        {
            AZ_TEST_START_ASSERTTEST;

            // MyFloat2
            m_srg->GetConstantArray<AZ::Vector4>(RHI::ShaderInputConstantIndex(13));

            AZ_TEST_STOP_ASSERTTEST(1);
        }
    }

    TEST_F(ShaderResourceGroupConstantBufferTests, TestErrorReporting_SetConstant_WrongNumberOfElements_SingleInput)
    {
        using namespace AZ;

        {
            AZ_TEST_START_ASSERTTEST;

            // MyBool2
            EXPECT_FALSE(m_srg->SetConstant<bool>(RHI::ShaderInputConstantIndex(1), false));

            AZ_TEST_STOP_ASSERTTEST(2);
        }
    }

    TEST_F(ShaderResourceGroupConstantBufferTests, TestErrorReporting_GetConstant_WrongNumberOfElements_SingleOutput)
    {
        using namespace AZ;

        {
            AZ_TEST_START_ASSERTTEST;

            // MyBool3
            EXPECT_FALSE(m_srg->GetConstant<bool>(RHI::ShaderInputConstantIndex(2)));

            AZ_TEST_STOP_ASSERTTEST(1);
        }
    }

    TEST_F(ShaderResourceGroupConstantBufferTests, TestCopyShaderResourceGroupData)
    {
        using namespace AZ;
        const RHI::ShaderInputConstantIndex inputIndex(4);

        // Check using inputIndex
        EXPECT_TRUE(m_srg->SetConstant(inputIndex, 51));

        auto testSrg2 = RPI::ShaderResourceGroup::Create(m_shaderAsset, AZ::RPI::DefaultSupervariantIndex, m_srgLayout->GetName());

        EXPECT_TRUE(testSrg2->CopyShaderResourceGroupData(*m_srg));
        EXPECT_EQ(m_srg->GetConstant<int32_t>(inputIndex), testSrg2->GetConstant<int32_t>(inputIndex));
    }

    TEST_F(ShaderResourceGroupConstantBufferTests, TestPartialCopyShaderResourceGroupData)
    {
        using namespace AZ;

        RHI::Ptr<RHI::ShaderResourceGroupLayout> srgLayout2 = RHI::ShaderResourceGroupLayout::Create();
        srgLayout2->SetName(Name("partial"));
        srgLayout2->SetBindingSlot(0);
        srgLayout2->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name("MyUint"), 0, sizeof(uint32_t), 0, 0 });
        srgLayout2->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name("MyBool1337"), sizeof(uint32_t), sizeof(bool), 0, 0 });
        srgLayout2->Finalize();

        auto testSrgShaderAsset2 = CreateTestShaderAsset(Uuid::CreateRandom(), srgLayout2);
        auto testSrg2 = RPI::ShaderResourceGroup::Create(testSrgShaderAsset2, AZ::RPI::DefaultSupervariantIndex, srgLayout2->GetName());

        const RHI::ShaderInputConstantIndex inputIndex(8);
        EXPECT_TRUE(m_srg->SetConstant(inputIndex, 23));
        EXPECT_TRUE(m_srg->SetConstant(RHI::ShaderInputConstantIndex{0}, false));

        EXPECT_FALSE(testSrg2->CopyShaderResourceGroupData(*m_srg));
        EXPECT_EQ(m_srg->GetConstant<uint32_t>(inputIndex), testSrg2->GetConstant<uint32_t>(RHI::ShaderInputConstantIndex{ 0 }));
    }
}
