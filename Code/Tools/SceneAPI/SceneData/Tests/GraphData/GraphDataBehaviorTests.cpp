/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/optional.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/ReflectionRegistrar.h>
#include <SceneAPI/SceneData/GraphData/CustomPropertyData.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexBitangentData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexColorData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexTangentData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexUVData.h>
#include <SceneAPI/SceneData/GraphData/AnimationData.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>
#include <SceneAPI/SceneData/GraphData/MaterialData.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>

extern "C" AZ_DLL_EXPORT void CleanUpSceneDataGenericClassInfo();

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            struct MockGraphData final
            {
                AZ_TYPE_INFO(MockGraphData, "{06996B36-E204-4ECC-9F3C-3D644B8CAE07}");

                MockGraphData() = default;
                ~MockGraphData() = default;

                static bool FillData(AZStd::any& data)
                {
                    if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::MeshData>())
                    {
                        auto* meshData = AZStd::any_cast<AZ::SceneData::GraphData::MeshData>(&data);
                        meshData->AddPosition(Vector3{1.0f, 1.1f, 2.2f});
                        meshData->AddPosition(Vector3{2.0f, 2.1f, 3.2f});
                        meshData->AddPosition(Vector3{3.0f, 3.1f, 4.2f});
                        meshData->AddPosition(Vector3{4.0f, 4.1f, 5.2f});
                        meshData->AddNormal(Vector3{0.1f, 0.2f, 0.3f});
                        meshData->AddNormal(Vector3{0.4f, 0.5f, 0.6f});
                        meshData->SetOriginalUnitSizeInMeters(10.0f);
                        meshData->SetUnitSizeInMeters(0.5f);
                        meshData->SetVertexIndexToControlPointIndexMap(0, 10);
                        meshData->SetVertexIndexToControlPointIndexMap(1, 11);
                        meshData->SetVertexIndexToControlPointIndexMap(2, 12);
                        meshData->SetVertexIndexToControlPointIndexMap(3, 13);
                        meshData->AddFace({0, 1, 2}, 1);
                        meshData->AddFace({3, 4, 5}, 2);
                        meshData->AddFace({6, 7, 8}, 3);
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::MeshVertexColorData>())
                    {
                        auto* colorData = AZStd::any_cast<AZ::SceneData::GraphData::MeshVertexColorData>(&data);
                        colorData->SetCustomName("mesh_vertex_color_data");
                        colorData->AppendColor(AZ::SceneAPI::DataTypes::Color{0.1f, 0.2f, 0.3f, 0.4f});
                        colorData->AppendColor(AZ::SceneAPI::DataTypes::Color{0.5f, 0.6f, 0.7f, 0.8f});
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::MeshVertexUVData>())
                    {
                        auto* uvData = AZStd::any_cast<AZ::SceneData::GraphData::MeshVertexUVData>(&data);
                        uvData->SetCustomName("mesh_vertex_uv_data");
                        uvData->AppendUV(AZ::Vector2{0.1f, 0.2f});
                        uvData->AppendUV(AZ::Vector2{0.3f, 0.4f});
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::MeshVertexBitangentData>())
                    {
                        auto* bitangentData = AZStd::any_cast<AZ::SceneData::GraphData::MeshVertexBitangentData>(&data);
                        bitangentData->AppendBitangent(AZ::Vector3{0.12f, 0.34f, 0.56f});
                        bitangentData->AppendBitangent(AZ::Vector3{0.77f, 0.88f, 0.99f});
                        bitangentData->SetGenerationMethod(AZ::SceneAPI::DataTypes::TangentGenerationMethod::FromSourceScene);
                        bitangentData->SetBitangentSetIndex(1);
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::MeshVertexTangentData>())
                    {
                        auto* tangentData = AZStd::any_cast<AZ::SceneData::GraphData::MeshVertexTangentData>(&data);
                        tangentData->AppendTangent(AZ::Vector4{0.12f, 0.34f, 0.56f, 0.78f});
                        tangentData->AppendTangent(AZ::Vector4{0.18f, 0.28f, 0.19f, 0.29f});
                        tangentData->AppendTangent(AZ::Vector4{0.21f, 0.43f, 0.65f, 0.87f});
                        tangentData->SetGenerationMethod(AZ::SceneAPI::DataTypes::TangentGenerationMethod::MikkT);
                        tangentData->SetTangentSetIndex(2);
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::AnimationData>())
                    {
                        auto* animationData = AZStd::any_cast<AZ::SceneData::GraphData::AnimationData>(&data);
                        animationData->ReserveKeyFrames(3);
                        animationData->AddKeyFrame(DataTypes::MatrixType::CreateFromValue(1.0));
                        animationData->AddKeyFrame(DataTypes::MatrixType::CreateFromValue(2.0));
                        animationData->AddKeyFrame(DataTypes::MatrixType::CreateFromValue(3.0));
                        animationData->SetTimeStepBetweenFrames(4.0);
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::BlendShapeAnimationData>())
                    {
                        auto* blendShapeAnimationData = AZStd::any_cast<AZ::SceneData::GraphData::BlendShapeAnimationData>(&data);
                        blendShapeAnimationData->SetBlendShapeName("mockBlendShapeName");
                        blendShapeAnimationData->ReserveKeyFrames(3);
                        blendShapeAnimationData->AddKeyFrame(1.0);
                        blendShapeAnimationData->AddKeyFrame(2.0);
                        blendShapeAnimationData->AddKeyFrame(3.0);
                        blendShapeAnimationData->SetTimeStepBetweenFrames(4.0);
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::BlendShapeData>())
                    {
                        auto* blendShapeData = AZStd::any_cast<AZ::SceneData::GraphData::BlendShapeData>(&data);
                        blendShapeData->AddPosition({ 1.0, 2.0, 3.0 });
                        blendShapeData->AddPosition({ 2.0, 3.0, 4.0 });
                        blendShapeData->AddPosition({ 3.0, 4.0, 5.0 });
                        blendShapeData->AddNormal({ 0.1, 0.2, 0.3 });
                        blendShapeData->AddNormal({ 0.2, 0.3, 0.4 });
                        blendShapeData->AddNormal({ 0.3, 0.4, 0.5 });
                        blendShapeData->AddTangentAndBitangent(Vector4{ 0.1f, 0.2f, 0.3f, 0.4f }, { 0.0, 0.1, 0.2 });
                        blendShapeData->AddTangentAndBitangent(Vector4{ 0.2f, 0.3f, 0.4f, 0.5f }, { 0.1, 0.2, 0.3 });
                        blendShapeData->AddTangentAndBitangent(Vector4{ 0.3f, 0.4f, 0.5f, 0.6f }, { 0.2, 0.3, 0.4 });
                        blendShapeData->AddUV(Vector2{ 0.9, 0.8 }, 0);
                        blendShapeData->AddUV(Vector2{ 0.7, 0.7 }, 1);
                        blendShapeData->AddUV(Vector2{ 0.6, 0.6 }, 2);
                        blendShapeData->AddColor(DataTypes::Color{ 0.1, 0.2, 0.3, 0.4 }, 0);
                        blendShapeData->AddColor(DataTypes::Color{ 0.2, 0.3, 0.4, 0.5 }, 1);
                        blendShapeData->AddColor(DataTypes::Color{ 0.3, 0.4, 0.5, 0.6 }, 2);
                        blendShapeData->AddFace({ 0, 1, 2 });
                        blendShapeData->AddFace({ 1, 2, 0 });
                        blendShapeData->AddFace({ 2, 0, 1 });
                        blendShapeData->SetVertexIndexToControlPointIndexMap(0, 1);
                        blendShapeData->SetVertexIndexToControlPointIndexMap(1, 2);
                        blendShapeData->SetVertexIndexToControlPointIndexMap(2, 0);
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::MaterialData>())
                    {
                        auto* materialDataData = AZStd::any_cast<AZ::SceneData::GraphData::MaterialData>(&data);
                        materialDataData->SetBaseColor(AZStd::make_optional(AZ::Vector3(0.1, 0.2, 0.3)));
                        materialDataData->SetDiffuseColor({ 0.3, 0.4, 0.5 });
                        materialDataData->SetEmissiveColor({ 0.4, 0.5, 0.6 });
                        materialDataData->SetEmissiveIntensity(AZStd::make_optional(0.789f));
                        materialDataData->SetMaterialName("TestMaterialName");
                        materialDataData->SetMetallicFactor(AZStd::make_optional(0.123f));
                        materialDataData->SetNoDraw(true);
                        materialDataData->SetOpacity(0.7);
                        materialDataData->SetRoughnessFactor(AZStd::make_optional(0.456f));
                        materialDataData->SetShininess(1.23);
                        materialDataData->SetSpecularColor({ 0.8, 0.9, 1.0 });
                        materialDataData->SetUseAOMap(AZStd::make_optional(true));
                        materialDataData->SetUseColorMap(AZStd::make_optional(true));
                        materialDataData->SetUseMetallicMap(AZStd::make_optional(true));
                        materialDataData->SetUseRoughnessMap(AZStd::make_optional(true));
                        materialDataData->SetUseEmissiveMap(AZStd::make_optional(true));
                        materialDataData->SetUniqueId(102938);
                        materialDataData->SetTexture(AZ::SceneAPI::DataTypes::IMaterialData::TextureMapType::AmbientOcclusion, "ambientocclusion");
                        materialDataData->SetTexture(AZ::SceneAPI::DataTypes::IMaterialData::TextureMapType::BaseColor, "basecolor");
                        materialDataData->SetTexture(AZ::SceneAPI::DataTypes::IMaterialData::TextureMapType::Bump, "bump");
                        materialDataData->SetTexture(AZ::SceneAPI::DataTypes::IMaterialData::TextureMapType::Diffuse, "diffuse");
                        materialDataData->SetTexture(AZ::SceneAPI::DataTypes::IMaterialData::TextureMapType::Emissive, "emissive");
                        materialDataData->SetTexture(AZ::SceneAPI::DataTypes::IMaterialData::TextureMapType::Metallic, "metallic");
                        materialDataData->SetTexture(AZ::SceneAPI::DataTypes::IMaterialData::TextureMapType::Normal, "normal");
                        materialDataData->SetTexture(AZ::SceneAPI::DataTypes::IMaterialData::TextureMapType::Roughness, "roughness");
                        materialDataData->SetTexture(AZ::SceneAPI::DataTypes::IMaterialData::TextureMapType::Specular, "specular");
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::BoneData>())
                    {
                        auto* boneData = AZStd::any_cast<AZ::SceneData::GraphData::BoneData>(&data);
                        boneData->SetWorldTransform(SceneAPI::DataTypes::MatrixType::CreateDiagonal({1.0, 2.0, 3.0}));
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::CustomPropertyData>())
                    {
                        AZ::SceneData::GraphData::CustomPropertyData::PropertyMap propertyMap;
                        propertyMap["a_string"] = AZStd::make_any<AZStd::string>("the string");
                        propertyMap["a_bool"] = AZStd::make_any<bool>(true);
                        propertyMap["a_int32"] = AZStd::make_any<int32_t>(aznumeric_cast<int32_t>(-32));
                        propertyMap["a_uint64"] = AZStd::make_any<AZ::u64>(aznumeric_cast<AZ::u64>(64));
                        propertyMap["a_float"] = AZStd::make_any<float>(aznumeric_cast<float>(12.34));
                        propertyMap["a_double"] = AZStd::make_any<double>(aznumeric_cast<double>(0.1234));
                        AZStd::any_cast<AZ::SceneData::GraphData::CustomPropertyData>(&data)->SetPropertyMap(propertyMap);
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::RootBoneData>())
                    {
                        auto* boneData = AZStd::any_cast<AZ::SceneData::GraphData::RootBoneData>(&data);
                        boneData->SetWorldTransform(SceneAPI::DataTypes::MatrixType::CreateDiagonal({2.0, 3.0, 4.0}));
                        return true;
                    }
                    else if (data.get_type_info().m_id == azrtti_typeid<AZ::SceneData::GraphData::TransformData>())
                    {
                        auto* transformData = AZStd::any_cast<AZ::SceneData::GraphData::TransformData>(&data);
                        transformData->SetMatrix(AZ::Matrix3x4::CreateDiagonal({1.0, 2.0, 3.0}));
                        return true;
                    }
                    return false;
                }

                static void Reflect(ReflectContext* context)
                {
                    BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
                    if (behaviorContext)
                    {
                        behaviorContext->Class<MockGraphData>()
                            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                            ->Attribute(AZ::Script::Attributes::Module, "scene")
                            ->Method("FillData", &MockGraphData::FillData);
                    }
                }
            };

            class GraphDataBehaviorScriptTest
                : public UnitTest::AllocatorsFixture
            {
            public:
                AZStd::unique_ptr<AZ::ScriptContext> m_scriptContext;
                AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
                AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;

                static void TestExpectTrue(bool value)
                {
                    EXPECT_TRUE(value);
                }

                static void TestExpectIntegerEquals(AZ::s64 lhs, AZ::s64 rhs)
                {
                    EXPECT_EQ(lhs, rhs);
                }

                static void TestExpectFloatEquals(float lhs, float rhs)
                {
                    EXPECT_EQ(lhs, rhs);
                }

                void SetUp() override
                {
                    UnitTest::AllocatorsFixture::SetUp();
                    AZ::NameDictionary::Create();

                    m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
                    AZ::SceneAPI::RegisterDataTypeReflection(m_serializeContext.get());

                    m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
                    m_behaviorContext->Method("TestExpectTrue", &TestExpectTrue);
                    m_behaviorContext->Method("TestExpectIntegerEquals", &TestExpectIntegerEquals);
                    m_behaviorContext->Method("TestExpectFloatEquals", &TestExpectFloatEquals);
                    MockGraphData::Reflect(m_behaviorContext.get());
                    AZ::MathReflect(m_behaviorContext.get());
                    AZ::SceneAPI::RegisterDataTypeBehaviorReflection(m_behaviorContext.get());

                    m_scriptContext = AZStd::make_unique<AZ::ScriptContext>();
                    m_scriptContext->BindTo(m_behaviorContext.get());
                }

                void TearDown() override
                {
                    m_scriptContext.reset();
                    m_serializeContext.reset();
                    m_behaviorContext.reset();

                    CleanUpSceneDataGenericClassInfo();

                    AZ::NameDictionary::Destroy();
                    UnitTest::AllocatorsFixture::TearDown();
                }

                void ExpectExecute(AZStd::string_view script)
                {
                    EXPECT_TRUE(m_scriptContext->Execute(script.data()));
                }
            };

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_MeshData_AccessWorks)
            {
                ExpectExecute("meshData = MeshData()");
                ExpectExecute("TestExpectTrue(meshData ~= nil)");
                ExpectExecute("MockGraphData.FillData(meshData)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetVertexCount(), 4)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(0).x, 1.0)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(0).y, 1.1)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(0).z, 2.2)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(1).x, 2.0)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(1).y, 2.1)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(1).z, 3.2)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(2).x, 3.0)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(2).y, 3.1)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(2).z, 4.2)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(3).x, 4.0)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(3).y, 4.1)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetPosition(3).z, 5.2)");
                ExpectExecute("TestExpectTrue(meshData:HasNormalData())");
                ExpectExecute("TestExpectFloatEquals(meshData:GetNormal(0).x, 0.1)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetNormal(0).y, 0.2)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetNormal(0).z, 0.3)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetNormal(1).x, 0.4)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetNormal(1).y, 0.5)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetNormal(1).z, 0.6)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetOriginalUnitSizeInMeters(), 10.0)");
                ExpectExecute("TestExpectFloatEquals(meshData:GetUnitSizeInMeters(), 0.5)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetUsedControlPointCount(), 4)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetControlPointIndex(0), 10)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetControlPointIndex(1), 11)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetControlPointIndex(2), 12)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetControlPointIndex(3), 13)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetUsedPointIndexForControlPoint(10), 0)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetUsedPointIndexForControlPoint(11), 1)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetUsedPointIndexForControlPoint(12), 2)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetUsedPointIndexForControlPoint(13), 3)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetUsedPointIndexForControlPoint(0), -1)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetFaceCount(), 3)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetVertexIndex(0, 0), 0)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetVertexIndex(0, 1), 1)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetVertexIndex(0, 2), 2)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetVertexIndex(2, 0), 6)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetVertexIndex(2, 1), 7)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetVertexIndex(2, 2), 8)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetFaceMaterialId(2), 3)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetFaceMaterialId(1), 2)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetFaceMaterialId(0), 1)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetFaceInfo(0):GetVertexIndex(0), 0)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetFaceInfo(0):GetVertexIndex(1), 1)");
                ExpectExecute("TestExpectIntegerEquals(meshData:GetFaceInfo(0):GetVertexIndex(2), 2)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_MeshVertexColorData_AccessWorks)
            {
                ExpectExecute("meshVertexColorData = MeshVertexColorData()");
                ExpectExecute("TestExpectTrue(meshVertexColorData ~= nil)");
                ExpectExecute("MockGraphData.FillData(meshVertexColorData)");
                ExpectExecute("TestExpectTrue(meshVertexColorData:GetCustomName() == 'mesh_vertex_color_data')");
                ExpectExecute("TestExpectIntegerEquals(meshVertexColorData:GetCount(), 2)");
                ExpectExecute("TestExpectFloatEquals(meshVertexColorData:GetColor(0).red, 0.1)");
                ExpectExecute("TestExpectFloatEquals(meshVertexColorData:GetColor(0).green, 0.2)");
                ExpectExecute("TestExpectFloatEquals(meshVertexColorData:GetColor(0).blue, 0.3)");
                ExpectExecute("TestExpectFloatEquals(meshVertexColorData:GetColor(0).alpha, 0.4)");
                ExpectExecute("colorOne = meshVertexColorData:GetColor(1)");
                ExpectExecute("TestExpectFloatEquals(colorOne.red, 0.5)");
                ExpectExecute("TestExpectFloatEquals(colorOne.green, 0.6)");
                ExpectExecute("TestExpectFloatEquals(colorOne.blue, 0.7)");
                ExpectExecute("TestExpectFloatEquals(colorOne.alpha, 0.8)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_MeshVertexUVData_AccessWorks)
            {
                ExpectExecute("meshVertexUVData = MeshVertexUVData()");
                ExpectExecute("TestExpectTrue(meshVertexUVData ~= nil)");
                ExpectExecute("MockGraphData.FillData(meshVertexUVData)");
                ExpectExecute("TestExpectTrue(meshVertexUVData:GetCustomName() == 'mesh_vertex_uv_data')");
                ExpectExecute("TestExpectIntegerEquals(meshVertexUVData:GetCount(), 2)");
                ExpectExecute("TestExpectFloatEquals(meshVertexUVData:GetUV(0).x, 0.1)");
                ExpectExecute("TestExpectFloatEquals(meshVertexUVData:GetUV(0).y, 0.2)");
                ExpectExecute("uvOne = meshVertexUVData:GetUV(1)");
                ExpectExecute("TestExpectFloatEquals(uvOne.x, 0.3)");
                ExpectExecute("TestExpectFloatEquals(uvOne.y, 0.4)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_MeshVertexBitangentData_AccessWorks)
            {
                ExpectExecute("meshVertexBitangentData = MeshVertexBitangentData()");
                ExpectExecute("TestExpectTrue(meshVertexBitangentData ~= nil)");
                ExpectExecute("MockGraphData.FillData(meshVertexBitangentData)");
                ExpectExecute("TestExpectIntegerEquals(meshVertexBitangentData:GetCount(), 2)");
                ExpectExecute("TestExpectFloatEquals(meshVertexBitangentData:GetBitangent(0).x, 0.12)");
                ExpectExecute("TestExpectFloatEquals(meshVertexBitangentData:GetBitangent(0).y, 0.34)");
                ExpectExecute("TestExpectFloatEquals(meshVertexBitangentData:GetBitangent(0).z, 0.56)");
                ExpectExecute("bitangentData = meshVertexBitangentData:GetBitangent(1)");
                ExpectExecute("TestExpectFloatEquals(bitangentData.x, 0.77)");
                ExpectExecute("TestExpectFloatEquals(bitangentData.y, 0.88)");
                ExpectExecute("TestExpectFloatEquals(bitangentData.z, 0.99)");
                ExpectExecute("TestExpectIntegerEquals(meshVertexBitangentData:GetBitangentSetIndex(), 1)");
                ExpectExecute("TestExpectTrue(meshVertexBitangentData:GetGenerationMethod(), MeshVertexBitangentData.FromSourceScene)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_MeshVertexTangentData_AccessWorks)
            {
                ExpectExecute("meshVertexTangentData = MeshVertexTangentData()");
                ExpectExecute("TestExpectTrue(meshVertexTangentData ~= nil)");
                ExpectExecute("MockGraphData.FillData(meshVertexTangentData)");
                ExpectExecute("TestExpectIntegerEquals(meshVertexTangentData:GetCount(), 3)");
                ExpectExecute("TestExpectFloatEquals(meshVertexTangentData:GetTangent(0).x, 0.12)");
                ExpectExecute("TestExpectFloatEquals(meshVertexTangentData:GetTangent(0).y, 0.34)");
                ExpectExecute("TestExpectFloatEquals(meshVertexTangentData:GetTangent(0).z, 0.56)");
                ExpectExecute("TestExpectFloatEquals(meshVertexTangentData:GetTangent(0).w, 0.78)");
                ExpectExecute("tangentData = meshVertexTangentData:GetTangent(1)");
                ExpectExecute("TestExpectFloatEquals(tangentData.x, 0.18)");
                ExpectExecute("TestExpectFloatEquals(tangentData.y, 0.28)");
                ExpectExecute("TestExpectFloatEquals(tangentData.z, 0.19)");
                ExpectExecute("TestExpectFloatEquals(tangentData.w, 0.29)");
                ExpectExecute("TestExpectIntegerEquals(meshVertexTangentData:GetTangentSetIndex(), 2)");
                ExpectExecute("TestExpectTrue(meshVertexTangentData:GetGenerationMethod(), MeshVertexTangentData.MikkT)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_AnimationData_AccessWorks)
            {
                ExpectExecute("animationData = AnimationData()");
                ExpectExecute("TestExpectTrue(animationData ~= nil)");
                ExpectExecute("MockGraphData.FillData(animationData)");
                ExpectExecute("TestExpectIntegerEquals(animationData:GetKeyFrameCount(), 3)");
                ExpectExecute("TestExpectFloatEquals(animationData:GetTimeStepBetweenFrames(), 4.0)");
                ExpectExecute("TestExpectFloatEquals(animationData:GetKeyFrame(0).basisX.x, 1.0)");
                ExpectExecute("TestExpectFloatEquals(animationData:GetKeyFrame(1).basisX.y, 2.0)");
                ExpectExecute("TestExpectFloatEquals(animationData:GetKeyFrame(2).basisX.z, 3.0)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_BlendShapeAnimationData_AccessWorks)
            {
                ExpectExecute("blendShapeAnimationData = BlendShapeAnimationData()");
                ExpectExecute("TestExpectTrue(blendShapeAnimationData ~= nil)");
                ExpectExecute("MockGraphData.FillData(blendShapeAnimationData)");
                ExpectExecute("TestExpectTrue(blendShapeAnimationData:GetBlendShapeName() == 'mockBlendShapeName')");
                ExpectExecute("TestExpectIntegerEquals(blendShapeAnimationData:GetKeyFrameCount(), 3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeAnimationData:GetKeyFrame(0), 1.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeAnimationData:GetKeyFrame(1), 2.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeAnimationData:GetKeyFrame(2), 3.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeAnimationData:GetTimeStepBetweenFrames(), 4.0)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_BlendShapeData_AccessWorks)
            {
                ExpectExecute("blendShapeData = BlendShapeData()");
                ExpectExecute("TestExpectTrue(blendShapeData ~= nil)");
                ExpectExecute("MockGraphData.FillData(blendShapeData)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetUsedControlPointCount(), 3)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetVertexCount(), 3)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetFaceCount(), 3)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetFaceVertexIndex(0, 2), 2)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetFaceVertexIndex(1, 0), 1)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetFaceVertexIndex(2, 1), 0)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetControlPointIndex(0), 1)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetControlPointIndex(1), 2)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetControlPointIndex(2), 0)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetUsedPointIndexForControlPoint(0), 2)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetUsedPointIndexForControlPoint(1), 0)");
                ExpectExecute("TestExpectIntegerEquals(blendShapeData:GetUsedPointIndexForControlPoint(2), 1)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetPosition(0).x, 1.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetPosition(0).y, 2.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetPosition(0).z, 3.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetPosition(1).x, 2.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetPosition(1).y, 3.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetPosition(1).z, 4.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetPosition(2).x, 3.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetPosition(2).y, 4.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetPosition(2).z, 5.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetNormal(0).x, 0.1)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetNormal(0).y, 0.2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetNormal(0).z, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetNormal(1).x, 0.2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetNormal(1).y, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetNormal(1).z, 0.4)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetNormal(2).x, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetNormal(2).y, 0.4)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetNormal(2).z, 0.5)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetFaceInfo(0):GetVertexIndex(0), 0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetFaceInfo(0):GetVertexIndex(1), 1)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetFaceInfo(0):GetVertexIndex(2), 2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetFaceInfo(1):GetVertexIndex(0), 1)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetFaceInfo(1):GetVertexIndex(1), 2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetFaceInfo(1):GetVertexIndex(2), 0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetFaceInfo(2):GetVertexIndex(0), 2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetFaceInfo(2):GetVertexIndex(1), 0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetFaceInfo(2):GetVertexIndex(2), 1)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetUV(0, 0).x, 0.9)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetUV(0, 0).y, 0.8)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetUV(0, 1).x, 0.7)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetUV(0, 1).y, 0.7)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetUV(0, 2).x, 0.6)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetUV(0, 2).y, 0.6)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(0, 0).red, 0.1)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(0, 0).green, 0.2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(0, 0).blue, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(0, 0).alpha, 0.4)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(1, 0).red, 0.2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(1, 0).green, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(1, 0).blue, 0.4)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(1, 0).alpha, 0.5)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(2, 0).red, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(2, 0).green, 0.4)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(2, 0).blue, 0.5)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetColor(2, 0).alpha, 0.6)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(0).x, 0.1)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(0).y, 0.2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(0).z, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(0).w, 0.4)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(1).x, 0.2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(1).y, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(1).z, 0.4)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(1).w, 0.5)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(2).x, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(2).y, 0.4)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(2).z, 0.5)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetTangent(2).w, 0.6)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetBitangent(0).x, 0.0)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetBitangent(0).y, 0.1)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetBitangent(0).z, 0.2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetBitangent(1).x, 0.1)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetBitangent(1).y, 0.2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetBitangent(1).z, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetBitangent(2).x, 0.2)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetBitangent(2).y, 0.3)");
                ExpectExecute("TestExpectFloatEquals(blendShapeData:GetBitangent(2).z, 0.4)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_MaterialData_AccessWorks)
            {
                ExpectExecute("materialData = MaterialData()");
                ExpectExecute("TestExpectTrue(materialData ~= nil)");
                ExpectExecute("TestExpectTrue(materialData:IsNoDraw() == false)");
                ExpectExecute("TestExpectTrue(materialData:GetUseColorMap() == false)");
                ExpectExecute("TestExpectTrue(materialData:GetUseMetallicMap() == false)");
                ExpectExecute("TestExpectTrue(materialData:GetUseRoughnessMap() == false)");
                ExpectExecute("TestExpectTrue(materialData:GetUseEmissiveMap() == false)");
                ExpectExecute("TestExpectTrue(materialData:GetUseAOMap() == false)");
                ExpectExecute("MockGraphData.FillData(materialData)");
                ExpectExecute("TestExpectTrue(materialData:IsNoDraw())");
                ExpectExecute("TestExpectTrue(materialData:GetUseColorMap())");
                ExpectExecute("TestExpectTrue(materialData:GetUseMetallicMap())");
                ExpectExecute("TestExpectTrue(materialData:GetUseRoughnessMap())");
                ExpectExecute("TestExpectTrue(materialData:GetUseEmissiveMap())");
                ExpectExecute("TestExpectTrue(materialData:GetUseAOMap())");
                ExpectExecute("TestExpectFloatEquals(materialData:GetMetallicFactor(), 0.123)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetRoughnessFactor(), 0.456)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetEmissiveIntensity(), 0.789)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetOpacity(), 0.7)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetShininess(), 1.23)");
                ExpectExecute("TestExpectTrue(materialData:GetMaterialName() == 'TestMaterialName')");
                ExpectExecute("TestExpectFloatEquals(materialData:GetBaseColor().x, 0.1)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetBaseColor().y, 0.2)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetBaseColor().z, 0.3)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetDiffuseColor().x, 0.3)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetDiffuseColor().y, 0.4)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetDiffuseColor().z, 0.5)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetEmissiveColor().x, 0.4)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetEmissiveColor().y, 0.5)");
                ExpectExecute("TestExpectFloatEquals(materialData:GetEmissiveColor().z, 0.6)");
                ExpectExecute("TestExpectIntegerEquals(materialData:GetUniqueId(), 102938)");
                ExpectExecute("TestExpectTrue(materialData:GetTexture(MaterialData.AmbientOcclusion) == 'ambientocclusion')");
                ExpectExecute("TestExpectTrue(materialData:GetTexture(MaterialData.Bump) == 'bump')");
                ExpectExecute("TestExpectTrue(materialData:GetTexture(MaterialData.Diffuse) == 'diffuse')");
                ExpectExecute("TestExpectTrue(materialData:GetTexture(MaterialData.Emissive) == 'emissive')");
                ExpectExecute("TestExpectTrue(materialData:GetTexture(MaterialData.Metallic) == 'metallic')");
                ExpectExecute("TestExpectTrue(materialData:GetTexture(MaterialData.Normal) == 'normal')");
                ExpectExecute("TestExpectTrue(materialData:GetTexture(MaterialData.Roughness) == 'roughness')");
                ExpectExecute("TestExpectTrue(materialData:GetTexture(MaterialData.Specular) == 'specular')");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_BoneData_AccessWorks)
            {
                ExpectExecute("boneData = BoneData()");
                ExpectExecute("TestExpectTrue(boneData ~= nil)");
                ExpectExecute("MockGraphData.FillData(boneData)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(0).x, 1.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(0).y, 0.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(0).z, 0.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(0).w, 0.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(1).x, 0.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(1).y, 2.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(1).z, 0.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(1).w, 0.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(2).x, 0.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(2).y, 0.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(2).z, 3.0)");
                ExpectExecute("TestExpectFloatEquals(boneData:GetWorldTransform():GetRow(2).w, 0.0)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_CustomPropertyData_AccessWorks)
            {
                ExpectExecute("customPropertyData = CustomPropertyData()");
                ExpectExecute("TestExpectTrue(customPropertyData ~= nil)");
                ExpectExecute("MockGraphData.FillData(customPropertyData)");
                ExpectExecute("TestExpectTrue(customPropertyData:GetPropertyMap():At('a_string') == 'the string')");
                ExpectExecute("TestExpectTrue(customPropertyData:GetPropertyMap():At('a_bool') == true)");
                ExpectExecute("TestExpectIntegerEquals(customPropertyData:GetPropertyMap():At('a_int32'), -32)");
                ExpectExecute("TestExpectIntegerEquals(customPropertyData:GetPropertyMap():At('a_uint64'), 64)");
                ExpectExecute("TestExpectFloatEquals(customPropertyData:GetPropertyMap():At('a_float'), 12.34)");
                ExpectExecute("TestExpectFloatEquals(customPropertyData:GetPropertyMap():At('a_double'), 0.1234)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_RootBoneData_AccessWorks)
            {
                ExpectExecute("rootBoneData = RootBoneData()");
                ExpectExecute("TestExpectTrue(rootBoneData ~= nil)");
                ExpectExecute("MockGraphData.FillData(rootBoneData)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(0).x, 2.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(0).y, 0.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(0).z, 0.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(0).w, 0.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(1).x, 0.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(1).y, 3.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(1).z, 0.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(1).w, 0.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(2).x, 0.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(2).y, 0.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(2).z, 4.0)");
                ExpectExecute("TestExpectFloatEquals(rootBoneData:GetWorldTransform():GetRow(2).w, 0.0)");
            }

            TEST_F(GraphDataBehaviorScriptTest, SceneGraph_TransformData_AccessWorks)
            {
                ExpectExecute("transformData = TransformData()");
                ExpectExecute("TestExpectTrue(transformData ~= nil)");
                ExpectExecute("MockGraphData.FillData(transformData)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(0).x, 1.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(0).y, 0.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(0).z, 0.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(0).w, 0.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(1).x, 0.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(1).y, 2.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(1).z, 0.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(1).w, 0.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(2).x, 0.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(2).y, 0.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(2).z, 3.0)");
                ExpectExecute("TestExpectFloatEquals(transformData.transform:GetRow(2).w, 0.0)");
            }
        }
    }
}
