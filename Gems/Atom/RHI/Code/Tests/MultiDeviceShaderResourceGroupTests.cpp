/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/ShaderResourceGroupData.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <Tests/Device.h>
#include <Tests/ShaderResourceGroup.h>

namespace UnitTest
{
    using namespace AZ;

    class MultiDeviceShaderResourceGroupTests : public MultiDeviceRHITestFixture
    {
    private:
        struct NestedData
        {
            float m_x;
            float m_y;
            float m_z;
        };

        struct ConstantBufferTest
        {
            float m_floatValue;
            uint32_t m_uintValue[3];
            float m_float4Value[4];
            NestedData m_nestedData[16];
            AZ::Matrix3x3 m_matrix3x3;
            AZ::Matrix4x4 m_matrix4x4;
            AZ::Matrix3x4 m_matrix3x4;
            AZ::Vector2 m_vector2;
            AZ::Vector3 m_vector3;
            AZ::Vector4 m_vector4;
        };

        const uint32_t ImageReadCount = 5;
        const uint32_t ImageReadWriteCount = 8;
        const uint32_t BufferConstantCount = 2;
        const uint32_t BufferReadCount = 2;
        const uint32_t BufferReadWriteCount = 2;

        AZStd::unique_ptr<SerializeContext> m_serializeContext;

    public:
        void SetUp() override
        {
            MultiDeviceRHITestFixture::SetUp();

            m_serializeContext = AZStd::make_unique<SerializeContext>();

            RHI::ReflectSystemComponent::Reflect(m_serializeContext.get());

            AZ::Name::Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            MultiDeviceRHITestFixture::TearDown();
        }

        RHI::ConstPtr<RHI::ShaderResourceGroupLayout> CreateLayout()
        {
            RHI::Ptr<RHI::ShaderResourceGroupLayout> layout = RHI::ShaderResourceGroupLayout::Create();
            layout->SetBindingSlot(0);
            layout->AddShaderInput(RHI::ShaderInputConstantDescriptor{
                Name("m_floatValue"), offsetof(ConstantBufferTest, m_floatValue), sizeof(ConstantBufferTest::m_floatValue), 0, 0 });
            layout->AddShaderInput(RHI::ShaderInputConstantDescriptor{
                Name("m_uintValue"), offsetof(ConstantBufferTest, m_uintValue), sizeof(ConstantBufferTest::m_uintValue), 0, 0 });
            layout->AddShaderInput(RHI::ShaderInputConstantDescriptor{
                Name("m_float4Value"), offsetof(ConstantBufferTest, m_float4Value), sizeof(ConstantBufferTest::m_float4Value), 0, 0 });
            layout->AddShaderInput(RHI::ShaderInputConstantDescriptor{
                Name("m_nestedData"), offsetof(ConstantBufferTest, m_nestedData), sizeof(ConstantBufferTest::m_nestedData), 0, 0 });
            layout->AddShaderInput(RHI::ShaderInputConstantDescriptor{
                Name("m_matrix3x3"),
                offsetof(ConstantBufferTest, m_matrix3x3),
                44,
                0,
                0 }); // Shader packs rows into 4 floats not 3, but doesn't include the last float on the last row, hence 44
            layout->AddShaderInput(
                RHI::ShaderInputConstantDescriptor{ Name("m_matrix4x4"), offsetof(ConstantBufferTest, m_matrix4x4), 64, 0, 0 });
            layout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name("m_matrix3x4"),
                                                                       offsetof(ConstantBufferTest, m_matrix3x4),
                                                                       48,
                                                                       0,
                                                                       0 }); // Shader packs rows into 4 floats not 3, hence its 48
            layout->AddShaderInput(
                RHI::ShaderInputConstantDescriptor{ Name("m_vector2"), offsetof(ConstantBufferTest, m_vector2), 8, 0, 0 });
            layout->AddShaderInput(
                RHI::ShaderInputConstantDescriptor{ Name("m_vector3"), offsetof(ConstantBufferTest, m_vector3), 12, 0, 0 });
            layout->AddShaderInput(
                RHI::ShaderInputConstantDescriptor{ Name("m_vector4"), offsetof(ConstantBufferTest, m_vector4), 16, 0, 0 });
            layout->AddShaderInput(RHI::ShaderInputImageDescriptor{
                Name("m_readImage"), RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, ImageReadCount, 1, 1 });
            layout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name("m_readWriteImage"),
                                                                    RHI::ShaderInputImageAccess::ReadWrite,
                                                                    RHI::ShaderInputImageType::Image2D,
                                                                    ImageReadWriteCount,
                                                                    2,
                                                                    2 });
            layout->AddShaderInput(RHI::ShaderInputBufferDescriptor{ Name("m_constantBuffer"),
                                                                     RHI::ShaderInputBufferAccess::Constant,
                                                                     RHI::ShaderInputBufferType::Constant,
                                                                     BufferConstantCount,
                                                                     UINT_MAX,
                                                                     3,
                                                                     3 });
            layout->AddShaderInput(RHI::ShaderInputBufferDescriptor{ Name("m_readBuffer"),
                                                                     RHI::ShaderInputBufferAccess::Read,
                                                                     RHI::ShaderInputBufferType::Structured,
                                                                     BufferReadCount,
                                                                     UINT_MAX,
                                                                     4,
                                                                     4 });
            layout->AddShaderInput(RHI::ShaderInputBufferDescriptor{ Name("m_readWriteBuffer"),
                                                                     RHI::ShaderInputBufferAccess::ReadWrite,
                                                                     RHI::ShaderInputBufferType::Typed,
                                                                     BufferReadWriteCount,
                                                                     UINT_MAX,
                                                                     5,
                                                                     5 });
            layout->AddStaticSampler(RHI::ShaderInputStaticSamplerDescriptor{
                Name("m_sampler"), RHI::SamplerState::CreateAnisotropic(16, RHI::AddressMode::Wrap), 6, 6 });

            bool success = layout->Finalize();
            if (!success)
            {
                return nullptr;
            }

            return layout;
        }

        void CreateSerializedLayout(RHI::ConstPtr<RHI::ShaderResourceGroupLayout>& serializedSrgLayout)
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout = CreateLayout();

            AZStd::vector<char, AZ::OSStdAllocator> srgBuffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char, AZ::OSStdAllocator>> outStream(&srgBuffer);

            {
                AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&outStream, *m_serializeContext.get(), AZ::ObjectStream::ST_BINARY);

                bool writeOK = objStream->WriteClass(srgLayout.get());
                ASSERT_TRUE(writeOK);

                bool finalizeOK = objStream->Finalize();
                ASSERT_TRUE(finalizeOK);
            }

            outStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

            AZ::ObjectStream::FilterDescriptor filterDesc;
            serializedSrgLayout =
                AZ::Utils::LoadObjectFromStream<RHI::ShaderResourceGroupLayout>(outStream, m_serializeContext.get(), filterDesc);
        }

        void TestShaderResourceGroupLayout()
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout = CreateLayout();
            TestShaderResourceGroupReflection(srgLayout);
        }

        void TestShaderResourceGroupLayoutSerialized()
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout;
            CreateSerializedLayout(srgLayout);
            TestShaderResourceGroupReflection(srgLayout);
        }

        void TestShaderResourceGroupPools()
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout = CreateLayout();

            {
                RHI::Ptr<RHI::ShaderResourceGroup> srgA = aznew AZ::RHI::ShaderResourceGroup;

                {
                    RHI::Ptr<RHI::ShaderResourceGroupPool> srgPool = aznew AZ::RHI::ShaderResourceGroupPool;

                    RHI::ShaderResourceGroupPoolDescriptor descriptor;
                    descriptor.m_budgetInBytes = 16;
                    descriptor.m_layout = srgLayout.get();

                    ASSERT_FALSE(srgPool->IsInitialized());
                    srgPool->Init(descriptor);
                    ASSERT_TRUE(srgPool->IsInitialized());
                    srgPool->Shutdown();
                    ASSERT_FALSE(srgPool->IsInitialized());
                    srgPool->Init(descriptor);
                    ASSERT_TRUE(srgPool->IsInitialized());
                    ASSERT_TRUE(srgPool->use_count() == 1);

                    ASSERT_TRUE(srgLayout->use_count() == (3 + DeviceCount));

                    RHI::Ptr<RHI::ShaderResourceGroup> srgB = aznew AZ::RHI::ShaderResourceGroup;
                    ASSERT_TRUE(srgA->GetPool() == nullptr);

                    srgPool->InitGroup(*srgA);
                    ASSERT_TRUE(srgA->IsInitialized());
                    ASSERT_TRUE(srgA->GetPool() == srgPool.get());
                    ASSERT_TRUE(srgPool->GetResourceCount() == 1);
                    srgA->Shutdown();
                    ASSERT_TRUE(srgPool->GetResourceCount() == 0);
                    ASSERT_TRUE(srgA->IsInitialized() == false);
                    ASSERT_TRUE(srgA->GetPool() == nullptr);
                    srgPool->InitGroup(*srgA);
                    ASSERT_TRUE(srgA->IsInitialized());
                    ASSERT_TRUE(srgA->GetPool() == srgPool.get());
                    srgPool->InitGroup(*srgB);

                    // Called to flush Resource::InvalidateViews() which has an increment/decrement for the use_count
                    RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

                    ASSERT_TRUE(srgA->use_count() == 1);
                    ASSERT_TRUE(srgB->use_count() == 1);
                    ASSERT_TRUE(srgPool->GetResourceCount() == 2);

                    {
                        uint32_t srgIndex = 0;

                        const RHI::ShaderResourceGroup* srgs[] = { srgA.get(), srgB.get() };

                        srgPool->ForEach<RHI::ShaderResourceGroup>(
                            [&srgIndex, &srgs](const RHI::ShaderResourceGroup& srg)
                            {
                                ASSERT_TRUE(srgs[srgIndex] == &srg);
                                srgIndex++;
                            });
                    }
                }

                ASSERT_TRUE(srgA->IsInitialized() == false);
                ASSERT_TRUE(srgA->GetPool() == nullptr);
            }

            ASSERT_TRUE(srgLayout->use_count() == 1);
            RHI::Ptr<RHI::ShaderResourceGroup> noopShaderResourceGroup = aznew AZ::RHI::ShaderResourceGroup;
        }

        void TestShaderResourceGroupReflection(const RHI::ConstPtr<RHI::ShaderResourceGroupLayout>& srgLayout)
        {
            EXPECT_EQ(srgLayout->GetGroupSizeForImages(), ImageReadCount + ImageReadWriteCount);
            EXPECT_EQ(srgLayout->GetGroupSizeForBuffers(), BufferConstantCount + BufferReadCount + BufferReadWriteCount);
            EXPECT_EQ(srgLayout->GetGroupInterval(RHI::ShaderInputImageIndex(0)).m_min, 0);
            EXPECT_EQ(srgLayout->GetGroupInterval(RHI::ShaderInputImageIndex(0)).m_max, ImageReadCount);
            EXPECT_EQ(srgLayout->GetGroupInterval(RHI::ShaderInputImageIndex(1)).m_min, ImageReadCount);
            EXPECT_EQ(srgLayout->GetGroupInterval(RHI::ShaderInputImageIndex(1)).m_max, ImageReadCount + ImageReadWriteCount);
            EXPECT_EQ(srgLayout->GetGroupInterval(RHI::ShaderInputBufferIndex(0)).m_min, 0);
            EXPECT_EQ(srgLayout->GetGroupInterval(RHI::ShaderInputBufferIndex(0)).m_max, BufferConstantCount);
            EXPECT_EQ(srgLayout->GetGroupInterval(RHI::ShaderInputBufferIndex(1)).m_min, BufferConstantCount);
            EXPECT_EQ(srgLayout->GetGroupInterval(RHI::ShaderInputBufferIndex(1)).m_max, BufferConstantCount + BufferReadCount);
            EXPECT_EQ(srgLayout->GetGroupInterval(RHI::ShaderInputBufferIndex(2)).m_min, BufferConstantCount + BufferReadCount);
            EXPECT_EQ(
                srgLayout->GetGroupInterval(RHI::ShaderInputBufferIndex(2)).m_max,
                BufferConstantCount + BufferReadCount + BufferReadWriteCount);
            EXPECT_EQ(srgLayout->use_count(), 1);

            RHI::ShaderInputImageIndex imageInputIndex = srgLayout->FindShaderInputImageIndex(Name("m_readImage"));
            EXPECT_EQ(imageInputIndex.GetIndex(), 0);

            imageInputIndex = srgLayout->FindShaderInputImageIndex(Name("m_readWriteImage"));
            EXPECT_EQ(imageInputIndex.GetIndex(), 1);

            RHI::ShaderInputBufferIndex bufferInputIndex = srgLayout->FindShaderInputBufferIndex(Name("m_constantBuffer"));
            EXPECT_EQ(bufferInputIndex.GetIndex(), 0);

            bufferInputIndex = srgLayout->FindShaderInputBufferIndex(Name("m_readBuffer"));
            EXPECT_EQ(bufferInputIndex.GetIndex(), 1);

            bufferInputIndex = srgLayout->FindShaderInputBufferIndex(Name("m_readWriteBuffer"));
            EXPECT_EQ(bufferInputIndex.GetIndex(), 2);

            RHI::ShaderInputConstantIndex floatValueIndex = srgLayout->FindShaderInputConstantIndex(Name("m_floatValue"));
            ASSERT_TRUE(floatValueIndex.GetIndex() == 0);
            RHI::ShaderInputConstantIndex uintValueIndex = srgLayout->FindShaderInputConstantIndex(Name("m_uintValue"));
            ASSERT_TRUE(uintValueIndex.GetIndex() == 1);
            RHI::ShaderInputConstantIndex float4ValueIndex = srgLayout->FindShaderInputConstantIndex(Name("m_float4Value"));
            ASSERT_TRUE(float4ValueIndex.GetIndex() == 2);
            RHI::ShaderInputConstantIndex nestedDataIndex = srgLayout->FindShaderInputConstantIndex(Name("m_nestedData"));
            ASSERT_TRUE(nestedDataIndex.GetIndex() == 3);
            RHI::ShaderInputConstantIndex matrix3x3Index = srgLayout->FindShaderInputConstantIndex(Name("m_matrix3x3"));
            ASSERT_TRUE(matrix3x3Index.GetIndex() == 4);
            RHI::ShaderInputConstantIndex matrix4x4Index = srgLayout->FindShaderInputConstantIndex(Name("m_matrix4x4"));
            ASSERT_TRUE(matrix4x4Index.GetIndex() == 5);
            RHI::ShaderInputConstantIndex matrix3x4Index = srgLayout->FindShaderInputConstantIndex(Name("m_matrix3x4"));
            ASSERT_TRUE(matrix3x4Index.GetIndex() == 6);

            RHI::Ptr<RHI::ShaderResourceGroupPool> srgPool = aznew AZ::RHI::ShaderResourceGroupPool;

            RHI::ShaderResourceGroupPoolDescriptor descriptor;
            descriptor.m_budgetInBytes = 16;
            descriptor.m_layout = srgLayout.get();
            srgPool->Init(descriptor);

            RHI::Ptr<RHI::ShaderResourceGroup> srg = aznew AZ::RHI::ShaderResourceGroup;
            srgPool->InitGroup(*srg);

            RHI::ShaderResourceGroupData srgData(*srg);

            float floatValue = 1.234f;
            srgData.SetConstant(floatValueIndex, floatValue);

            AZStd::array<uint32_t, 3> uintValues = { 5, 6, 7 };
            srgData.SetConstant(uintValueIndex, uintValues);

            AZStd::array<float, 4> float4Values = { 10.1f, 11.2f, 12.3f, 14.4f };
            srgData.SetConstant(float4ValueIndex, float4Values);

            NestedData nestedData[16];
            for (uint32_t i = 0; i < 16; ++i)
            {
                nestedData[i].m_x = (float)i * 2;
                nestedData[i].m_y = (float)i * 3;
                nestedData[i].m_z = (float)i * 4;
            }

            // Write the first one as a single element.
            srgData.SetConstantRaw(nestedDataIndex, &nestedData[0], sizeof(NestedData));

            // Write the second one as an element with an offset.
            srgData.SetConstantRaw(nestedDataIndex, &nestedData[1], sizeof(NestedData), sizeof(NestedData));

            // Write the next 13 as an array.
            srgData.SetConstantRaw(nestedDataIndex, nestedData + 2, sizeof(NestedData) * 2, sizeof(NestedData) * 13);

            // Write the last one as a single value with an offset.
            srgData.SetConstantRaw(nestedDataIndex, &nestedData[15], sizeof(NestedData) * 15, sizeof(NestedData));

            float floatValueResult = srgData.GetConstant<float>(floatValueIndex);
            EXPECT_EQ(floatValueResult, floatValue);

            const auto ValidateFloat4Values = [&]()
            {
                AZStd::span<const float> float4ValueResult =
                    srgData.GetConstantArray<float>(float4ValueIndex);
                EXPECT_EQ(float4ValueResult.size(), 4);
                EXPECT_EQ(float4ValueResult[0], float4Values[0]);
                EXPECT_EQ(float4ValueResult[1], float4Values[1]);
                EXPECT_EQ(float4ValueResult[2], float4Values[2]);
                EXPECT_EQ(float4ValueResult[3], float4Values[3]);
            };

            AZStd::span<const uint32_t> uintValuesResult =
                srgData.GetConstantArray<uint32_t>(uintValueIndex);
            EXPECT_EQ(uintValuesResult.size(), 3);
            EXPECT_EQ(uintValuesResult[0], uintValues[0]);
            EXPECT_EQ(uintValuesResult[1], uintValues[1]);
            EXPECT_EQ(uintValuesResult[2], uintValues[2]);

            AZStd::span<const NestedData> nestedDataResult =
                srgData.GetConstantArray<NestedData>(nestedDataIndex);
            EXPECT_EQ(nestedDataResult.size(), 16);

            ValidateFloat4Values();

            for (uint32_t i = 0; i < 16; ++i)
            {
                EXPECT_EQ(nestedDataResult[i].m_x, nestedData[i].m_x);
                EXPECT_EQ(nestedDataResult[i].m_y, nestedData[i].m_y);
                EXPECT_EQ(nestedDataResult[i].m_z, nestedData[i].m_z);
            }

            // SetConstant Matrix tests
            float matrixValue[16] = {
                1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f
            };

            // SetConstant matrix of type Matrix3x3
            const AZ::Matrix3x3& mat3x3Values = AZ::Matrix3x3::CreateFromRowMajorFloat9(matrixValue);
            srgData.SetConstant(matrix3x3Index, mat3x3Values);
            EXPECT_EQ(srgData.GetConstant<AZ::Matrix3x3>(matrix3x3Index), mat3x3Values);

            // SetConstant matrix of type Matrix3x4
            const AZ::Matrix3x4& mat3x4Values = AZ::Matrix3x4::CreateFromRowMajorFloat12(matrixValue);
            srgData.SetConstant(matrix3x4Index, mat3x4Values);
            EXPECT_EQ(srgData.GetConstant<AZ::Matrix3x4>(matrix3x4Index), mat3x4Values);

            // SetConstant matrix of type Matrix4x4
            const AZ::Matrix4x4& mat4x4Values = AZ::Matrix4x4::CreateFromRowMajorFloat16(matrixValue);
            srgData.SetConstant(matrix4x4Index, mat4x4Values);
            EXPECT_EQ(srgData.GetConstant<AZ::Matrix4x4>(matrix4x4Index), mat4x4Values);

            // Reset the constant matrix3x4Index with identity
            srgData.SetConstant(matrix3x4Index, AZ::Matrix3x4::CreateIdentity());

            // SetConstant matrix rows, sets 3 rows from 4x4 matrix (which becomes 3x4 matrix)
            srgData.SetConstantMatrixRows(matrix3x4Index, mat4x4Values, 3);
            EXPECT_EQ(srgData.GetConstant<AZ::Matrix3x4>(matrix3x4Index), mat3x4Values);

            // Reset the constant matrix3x3Index with identity
            srgData.SetConstant(matrix3x3Index, AZ::Matrix3x3::CreateIdentity());

            srgData.SetConstantMatrixRows(matrix3x3Index, mat3x3Values, 3);
            EXPECT_EQ(srgData.GetConstant<AZ::Matrix3x3>(matrix3x3Index), mat3x3Values);

            // Reset the constant matrix4x4Index with identity
            srgData.SetConstant(matrix4x4Index, AZ::Matrix4x4::CreateIdentity());

            srgData.SetConstantMatrixRows(matrix4x4Index, mat4x4Values, 4);
            EXPECT_EQ(srgData.GetConstant<AZ::Matrix4x4>(matrix4x4Index), mat4x4Values);

            // SetConstant
            {
                // Attempt to a larger amount of data than is supported.
                AZ_TEST_START_ASSERTTEST;
                srgData.SetConstant(floatValueIndex, AZ::Vector4::CreateOne());
                AZ_TEST_STOP_ASSERTTEST(DeviceCount + 1);

                EXPECT_EQ(srgData.GetConstant<float>(floatValueIndex), floatValue);

                // Attempt to assign a smaller amount of data than is supported.
                AZ_TEST_START_ASSERTTEST;
                srgData.SetConstant(floatValueIndex, uint8_t(0));
                AZ_TEST_STOP_ASSERTTEST(DeviceCount + 1);

                EXPECT_EQ(srgData.GetConstant<float>(floatValueIndex), floatValue);
            }

            // SetConstant (ArrayIndex)
            {
                // Assign index that overflows array.
                AZ_TEST_START_ASSERTTEST;
                srgData.SetConstant(float4ValueIndex, 5.0f, 5);
                AZ_TEST_STOP_ASSERTTEST(DeviceCount + 1);

                ValidateFloat4Values();

                // Assign index where alignment doesn't match up.
                struct Test
                {
                    uint16_t m_a = 0;
                    uint16_t m_b = 1;
                    uint16_t m_c = 2;
                };

                AZ_TEST_START_ASSERTTEST;
                srgData.SetConstant(float4ValueIndex, Test(), 1);
                AZ_TEST_STOP_ASSERTTEST(DeviceCount + 1);

                ValidateFloat4Values();

                // Finally, assign a valid value and make sure it get assigned.
                float4Values[1] = 99.0f;
                srgData.SetConstant(float4ValueIndex, float4Values[1], 1);
                ValidateFloat4Values();
            }

            // SetConstantArray
            {
                // Attempt to a larger amount of data than is supported.
                float float6Values[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
                AZ_TEST_START_ASSERTTEST;
                srgData.SetConstantArray<float>(float4ValueIndex, { float6Values, AZ_ARRAY_SIZE(float6Values) });
                AZ_TEST_STOP_ASSERTTEST(DeviceCount + 1);

                ValidateFloat4Values();

                // Attempt to assign a smaller amount of data than is supported.
                float float1Value[] = { 5.0f };
                AZ_TEST_START_ASSERTTEST;
                srgData.SetConstantArray<float>(float4ValueIndex, { float1Value, AZ_ARRAY_SIZE(float1Value) });
                AZ_TEST_STOP_ASSERTTEST(DeviceCount + 1);

                ValidateFloat4Values();
            }
        }
    };

    namespace MultiDevice
    {

        RHI::ShaderResourceGroupData PrepareSRGData(const RHI::ConstPtr<RHI::ShaderResourceGroupLayout>& srgLayout)
        {
            RHI::Ptr<RHI::ShaderResourceGroupPool> srgPool = aznew AZ::RHI::ShaderResourceGroupPool;

            RHI::ShaderResourceGroupPoolDescriptor descriptor;
            descriptor.m_layout = srgLayout.get();
            srgPool->Init(descriptor);

            RHI::Ptr<RHI::ShaderResourceGroup> srg = aznew AZ::RHI::ShaderResourceGroup;
            srgPool->InitGroup(*srg);

            RHI::ShaderResourceGroupData srgData(*srg);
            return srgData;
        }

        void TestSetConstantVectorsValidCase(const RHI::ConstPtr<RHI::ShaderResourceGroupLayout>& srgLayout)
        {
            const RHI::ShaderInputConstantIndex vector2index = srgLayout->FindShaderInputConstantIndex(Name("m_vector2"));
            EXPECT_EQ(vector2index.GetIndex(), 7);
            const RHI::ShaderInputConstantIndex vector3index = srgLayout->FindShaderInputConstantIndex(Name("m_vector3"));
            EXPECT_EQ(vector3index.GetIndex(), 8);
            const RHI::ShaderInputConstantIndex vector4index = srgLayout->FindShaderInputConstantIndex(Name("m_vector4"));
            EXPECT_EQ(vector4index.GetIndex(), 9);

            RHI::ShaderResourceGroupData srgData = PrepareSRGData(srgLayout);

            const float vector2values[2] = { 1.0f, 2.0f };
            const Vector2 vector2 = Vector2::CreateFromFloat2(vector2values);

            const float vector3values[3] = { 3.0f, 4.0f, 5.0f };
            const Vector3 vector3 = Vector3::CreateFromFloat3(vector3values);

            const float vector4values[4] = { 6.0f, 7.0f, 8.0f, 9.0f };
            const Vector4 vector4 = Vector4::CreateFromFloat4(vector4values);

            EXPECT_TRUE(srgData.SetConstant(vector2index, vector2));
            AZStd::span<const uint8_t> resultVector2 = srgData.GetConstantRaw(vector2index);
            const Vector2 vector2result = *reinterpret_cast<const Vector2*>(resultVector2.data());
            EXPECT_EQ(vector2result, vector2);

            EXPECT_TRUE(srgData.SetConstant(vector3index, vector3));
            AZStd::span<const uint8_t> resutVector3 = srgData.GetConstantRaw(vector3index);
            const Vector3 vector3result = *reinterpret_cast<const Vector3*>(resutVector3.data());
            EXPECT_EQ(vector3result, vector3);

            EXPECT_TRUE(srgData.SetConstant(vector4index, vector4));
            AZStd::span<const uint8_t> resutVector4 = srgData.GetConstantRaw(vector4index);
            const Vector4 vector4result = *reinterpret_cast<const Vector4*>(resutVector4.data());
            EXPECT_EQ(vector4result, vector4);
        }
        void TestSetConstantVectorsInvalidCase(const RHI::ConstPtr<RHI::ShaderResourceGroupLayout>& srgLayout)
        {
            const RHI::ShaderInputConstantIndex vector2index = srgLayout->FindShaderInputConstantIndex(Name("m_vector2"));
            EXPECT_EQ(vector2index.GetIndex(), 7);
            const RHI::ShaderInputConstantIndex vector3index = srgLayout->FindShaderInputConstantIndex(Name("m_vector3"));
            EXPECT_EQ(vector3index.GetIndex(), 8);
            const RHI::ShaderInputConstantIndex vector4index = srgLayout->FindShaderInputConstantIndex(Name("m_vector4"));
            EXPECT_EQ(vector4index.GetIndex(), 9);

            RHI::ShaderResourceGroupData srgData = PrepareSRGData(srgLayout);

            const float vector2values[2] = { 1.0f, 2.0f };
            const Vector2 vector2 = Vector2::CreateFromFloat2(vector2values);

            const float vector3values[3] = { 1.0f, 2.0f, 3.0f };
            const Vector3 vector3 = Vector3::CreateFromFloat3(vector3values);

            const float vector4values[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
            const Vector4 vector4 = Vector4::CreateFromFloat4(vector4values);

            // Reset constant vector2index to zero
            EXPECT_TRUE(srgData.SetConstant(vector2index, vector2.CreateZero()));

            AZ_TEST_START_ASSERTTEST;
            EXPECT_FALSE(srgData.SetConstant(vector2index, vector3));
            AZ_TEST_STOP_ASSERTTEST(DeviceCount + 1);
            AZStd::span<const uint8_t> resutV3 = srgData.GetConstantRaw(vector2index);
            const Vector3 v3result = *reinterpret_cast<const Vector3*>(resutV3.data());
            EXPECT_NE(v3result, vector3);

            // Reset constant vector3index to zero
            EXPECT_TRUE(srgData.SetConstant(vector3index, vector3.CreateZero()));

            AZ_TEST_START_ASSERTTEST;
            EXPECT_FALSE(srgData.SetConstant(vector3index, vector4));
            AZ_TEST_STOP_ASSERTTEST(DeviceCount + 1);
            AZStd::span<const uint8_t> resutV4 = srgData.GetConstantRaw(vector3index);
            const Vector4 v4result = *reinterpret_cast<const Vector4*>(resutV4.data());
            EXPECT_NE(v4result, vector4);

            // Reset constant vector4index to zero
            EXPECT_TRUE(srgData.SetConstant(vector4index, vector4.CreateZero()));

            AZ_TEST_START_ASSERTTEST;
            EXPECT_FALSE(srgData.SetConstant(vector4index, vector3));
            AZ_TEST_STOP_ASSERTTEST(DeviceCount + 1);
            AZStd::span<const uint8_t> resutV3FromIndex4 = srgData.GetConstantRaw(vector4index);
            const Vector4 v4resultFromIndex4 = *reinterpret_cast<const Vector4*>(resutV3FromIndex4.data());
            EXPECT_NE(v4resultFromIndex4, vector4);
        }

        void TestGetConstantVectorsValidCase(const RHI::ConstPtr<RHI::ShaderResourceGroupLayout>& srgLayout)
        {
            const RHI::ShaderInputConstantIndex vector2index = srgLayout->FindShaderInputConstantIndex(Name("m_vector2"));
            EXPECT_EQ(vector2index.GetIndex(), 7);
            const RHI::ShaderInputConstantIndex vector3index = srgLayout->FindShaderInputConstantIndex(Name("m_vector3"));
            EXPECT_EQ(vector3index.GetIndex(), 8);
            const RHI::ShaderInputConstantIndex vector4index = srgLayout->FindShaderInputConstantIndex(Name("m_vector4"));
            EXPECT_EQ(vector4index.GetIndex(), 9);

            RHI::ShaderResourceGroupData srgData = PrepareSRGData(srgLayout);

            const float vector2values[2] = { 1.0f, 2.0f };
            const Vector2 vector2 = Vector2::CreateFromFloat2(vector2values);

            const float vector3values[3] = { 1.0f, 2.0f, 3.0f };
            const Vector3 vector3 = Vector3::CreateFromFloat3(vector3values);

            const float vector4values[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
            const Vector4 vector4 = Vector4::CreateFromFloat4(vector4values);

            EXPECT_TRUE(srgData.SetConstantRaw(vector2index, &vector2, 8));
            const Vector2 vector2result = srgData.GetConstant<Vector2>(vector2index);
            EXPECT_EQ(vector2result, vector2);

            EXPECT_TRUE(srgData.SetConstantRaw(vector3index, &vector3, 12));
            const Vector3 vector3result = srgData.GetConstant<Vector3>(vector3index);
            EXPECT_EQ(vector3result, vector3);

            EXPECT_TRUE(srgData.SetConstantRaw(vector4index, &vector4, 16));
            const Vector4 vector4result = srgData.GetConstant<Vector4>(vector4index);
            EXPECT_EQ(vector4result, vector4);
        }
        void TestGetConstantVectorsInvalidCase(const RHI::ConstPtr<RHI::ShaderResourceGroupLayout>& srgLayout)
        {
            const RHI::ShaderInputConstantIndex vector2index = srgLayout->FindShaderInputConstantIndex(Name("m_vector2"));
            EXPECT_EQ(vector2index.GetIndex(), 7);
            const RHI::ShaderInputConstantIndex vector3index = srgLayout->FindShaderInputConstantIndex(Name("m_vector3"));
            EXPECT_EQ(vector3index.GetIndex(), 8);
            const RHI::ShaderInputConstantIndex vector4index = srgLayout->FindShaderInputConstantIndex(Name("m_vector4"));
            EXPECT_EQ(vector4index.GetIndex(), 9);

            RHI::ShaderResourceGroupData srgData = PrepareSRGData(srgLayout);

            const float vector2values[2] = { 1.0f, 2.0f };
            const Vector2 vector2 = Vector2::CreateFromFloat2(vector2values);

            const float vector3values[3] = { 1.0f, 2.0f, 3.0f };
            const Vector3 vector3 = Vector3::CreateFromFloat3(vector3values);

            const float vector4values[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
            const Vector4 vector4 = Vector4::CreateFromFloat4(vector4values);

            // Invalid cases for GetConstant
            EXPECT_TRUE(srgData.SetConstantRaw(vector2index, &vector2, 8));
            AZ_TEST_START_ASSERTTEST;
            const Vector3 invalidVector3result = srgData.GetConstant<Vector3>(vector2index);
            EXPECT_NE(invalidVector3result, vector3);
            AZ_TEST_STOP_ASSERTTEST(1);

            EXPECT_TRUE(srgData.SetConstantRaw(vector3index, &vector3, 12));
            AZ_TEST_START_ASSERTTEST;
            const Vector4 invalidVector4result = srgData.GetConstant<Vector4>(vector3index);
            EXPECT_NE(invalidVector4result, vector4);
            AZ_TEST_STOP_ASSERTTEST(1);

            EXPECT_TRUE(srgData.SetConstantRaw(vector4index, &vector4, 16));
            AZ_TEST_START_ASSERTTEST;
            const Vector2 invalidVector2result = srgData.GetConstant<Vector2>(vector4index);
            EXPECT_NE(invalidVector2result, vector2);
            AZ_TEST_STOP_ASSERTTEST(1);
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, TestShaderResourceGroupLayout)
        {
            TestShaderResourceGroupLayout();
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, TestShaderResourceGroupLayoutSerialized)
        {
            TestShaderResourceGroupLayoutSerialized();
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, TestShaderResourceGroupPools)
        {
            TestShaderResourceGroupPools();
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, SRGDataSetConstant_Vectors_ValidOutput)
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout = CreateLayout();

            TestSetConstantVectorsValidCase(srgLayout);
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, SRGDataSetConstant_Vectors_InvalidOutput)
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout = CreateLayout();

            TestSetConstantVectorsInvalidCase(srgLayout);
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, SRGDataGetConstant_Vectors_ValidOutput)
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout = CreateLayout();

            TestGetConstantVectorsValidCase(srgLayout);
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, SRGDataGetConstant_Vectors_InvalidOutput)
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout = CreateLayout();

            TestGetConstantVectorsInvalidCase(srgLayout);
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, SRGDataSetConstant_Vectors_ValidOutput_Serialized)
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout;
            CreateSerializedLayout(srgLayout);

            TestSetConstantVectorsValidCase(srgLayout);
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, SRGDataSetConstant_Vectors_InvalidOutput_Serialized)
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout;
            CreateSerializedLayout(srgLayout);

            TestSetConstantVectorsInvalidCase(srgLayout);
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, SRGDataGetConstant_Vectors_ValidOutput_Serialized)
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout;
            CreateSerializedLayout(srgLayout);

            TestGetConstantVectorsValidCase(srgLayout);
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, SRGDataGetConstant_Vectors_InvalidOutput_Serialized)
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout;
            CreateSerializedLayout(srgLayout);

            TestGetConstantVectorsInvalidCase(srgLayout);
        }

        TEST_F(MultiDeviceShaderResourceGroupTests, TestShaderResourceGroupLayoutHash)
        {
            const Name imageName("m_image");
            const Name bufferName("m_buffer");
            const Name samplerName("m_sampler");
            const Name constantBufferName("m_constantBuffer");

            RHI::Ptr<RHI::ShaderResourceGroupLayout> layout = RHI::ShaderResourceGroupLayout::Create();
            layout->SetBindingSlot(0);
            layout->AddShaderInput(
                RHI::ShaderInputImageDescriptor{ imageName, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1,
                1
                });
            layout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                bufferName, RHI::ShaderInputBufferAccess::Constant, RHI::ShaderInputBufferType::Constant, 2, UINT_MAX, 3, 3 });
            layout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                samplerName, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Structured, 3, UINT_MAX, 4, 4 });
            layout->AddStaticSampler(RHI::ShaderInputStaticSamplerDescriptor{
                constantBufferName, RHI::SamplerState::CreateAnisotropic(16, RHI::AddressMode::Wrap), 6, 6 });
            EXPECT_TRUE(layout->Finalize());

            {
                // Test change name of one shader input
                RHI::Ptr<RHI::ShaderResourceGroupLayout> otherLayout = RHI::ShaderResourceGroupLayout::Create();
                otherLayout->SetBindingSlot(0);
                otherLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{
                    imageName, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1, 1 });
                otherLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                    bufferName, RHI::ShaderInputBufferAccess::Constant, RHI::ShaderInputBufferType::Constant, 2, UINT_MAX, 3, 3 });
                otherLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                    samplerName, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Structured, 3, UINT_MAX, 4, 4 });
                otherLayout->AddStaticSampler(RHI::ShaderInputStaticSamplerDescriptor{
                    Name{ "m_constantBuffer2" }, RHI::SamplerState::CreateAnisotropic(16, RHI::AddressMode::Wrap), 6, 6 });
                EXPECT_TRUE(otherLayout->Finalize());
                EXPECT_NE(otherLayout->GetHash(), layout->GetHash());
            }

            {
                // Test change of binding slot
                RHI::Ptr<RHI::ShaderResourceGroupLayout> otherLayout = RHI::ShaderResourceGroupLayout::Create();
                otherLayout->SetBindingSlot(1);
                otherLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{
                    imageName, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1, 1 });
                otherLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                    bufferName, RHI::ShaderInputBufferAccess::Constant, RHI::ShaderInputBufferType::Constant, 2, UINT_MAX, 3, 3 });
                otherLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                    samplerName, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Structured, 3, UINT_MAX, 4, 4 });
                otherLayout->AddStaticSampler(RHI::ShaderInputStaticSamplerDescriptor{
                    constantBufferName, RHI::SamplerState::CreateAnisotropic(16, RHI::AddressMode::Wrap), 6, 6 });
                EXPECT_TRUE(otherLayout->Finalize());
                EXPECT_NE(otherLayout->GetHash(), layout->GetHash());
            }

            {
                // Test adding constants
                RHI::Ptr<RHI::ShaderResourceGroupLayout> otherLayout = RHI::ShaderResourceGroupLayout::Create();
                otherLayout->SetBindingSlot(0);
                otherLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{
                    imageName, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1, 1 });
                otherLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                    bufferName, RHI::ShaderInputBufferAccess::Constant, RHI::ShaderInputBufferType::Constant, 2, UINT_MAX, 3, 3 });
                otherLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                    samplerName, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Structured, 3, UINT_MAX, 4, 4 });
                otherLayout->AddStaticSampler(RHI::ShaderInputStaticSamplerDescriptor{
                    constantBufferName, RHI::SamplerState::CreateAnisotropic(16, RHI::AddressMode::Wrap), 6, 6 });
                otherLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name("m_floatValue"), 0, 4, 0, 0 });
                EXPECT_TRUE(otherLayout->Finalize());
                EXPECT_NE(otherLayout->GetHash(), layout->GetHash());
            }

            {
                // Test adding shader variant key fallback
                RHI::Ptr<RHI::ShaderResourceGroupLayout> otherLayout = RHI::ShaderResourceGroupLayout::Create();
                otherLayout->SetBindingSlot(0);
                otherLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{
                    imageName, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1, 1 });
                otherLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                    bufferName, RHI::ShaderInputBufferAccess::Constant, RHI::ShaderInputBufferType::Constant, 2, UINT_MAX, 3, 3 });
                otherLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                    samplerName, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Structured, 3, UINT_MAX, 4, 4 });
                otherLayout->AddStaticSampler(RHI::ShaderInputStaticSamplerDescriptor{
                    constantBufferName, RHI::SamplerState::CreateAnisotropic(16, RHI::AddressMode::Wrap), 6, 6 });
                otherLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name("m_floatValue"), 0, 4, 0, 0 });
                otherLayout->SetShaderVariantKeyFallback(Name{ "m_floatValue" }, 1);
                EXPECT_TRUE(otherLayout->Finalize());
                EXPECT_NE(otherLayout->GetHash(), layout->GetHash());
            }
        }
    } // namespace MultiDevice
} // namespace UnitTest
