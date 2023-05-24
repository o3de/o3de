/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>
#include <Common/JsonTestUtils.h>
#include <Common/ErrorMessageFinder.h>
#include <Common/ShaderAssetTestUtils.h>
#include <AzCore/Script/ScriptAsset.h>
#include <Atom/RPI.Edit/Material/LuaMaterialFunctorSourceData.h>
#include <Atom/RPI.Reflect/Material/LuaMaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Material/MaterialAssetTestUtils.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class LuaMaterialFunctorTests
        : public RPITestFixture
    {
    public:
        static void AddLuaFunctor(MaterialTypeAssetCreator& materialTypeCreator, const AZStd::string& script, Name materialPipelineName = MaterialPipelineNone)
        {
            // See also MaterialTypeSourceData::AddFunctors

            LuaMaterialFunctorSourceData functorSourceData;
            functorSourceData.m_luaScript = script;

            MaterialNameContext nameContext;

            MaterialFunctorSourceData::RuntimeContext createFunctorContext{
                "Dummy.materialtype",
                materialTypeCreator.GetMaterialPropertiesLayout(materialPipelineName),
                (materialPipelineName == MaterialPipelineNone) ? materialTypeCreator.GetMaterialShaderResourceGroupLayout() : nullptr,
                &nameContext
            };

            MaterialFunctorSourceData::FunctorResult result = functorSourceData.CreateFunctor(createFunctorContext);
            EXPECT_TRUE(result.IsSuccess());

            if (result.IsSuccess())
            {
                materialTypeCreator.AddMaterialFunctor(result.GetValue(), materialPipelineName);

                for (auto& shaderOption : functorSourceData.GetShaderOptionDependencies())
                {
                    materialTypeCreator.ClaimShaderOptionOwnership(shaderOption);
                }
            }
        }


    protected:
        void SetUp() override
        {
            RPITestFixture::SetUp();
        }

        void TearDown() override
        {
            RPITestFixture::TearDown();
        }

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> CreateCommonTestShaderOptionsLayout()
        {
            AZStd::vector<RPI::ShaderOptionValuePair> boolOptionValues = CreateBoolShaderOptionValues();
            AZStd::vector<RPI::ShaderOptionValuePair> intRangeOptionValues = CreateIntRangeShaderOptionValues(0, 15);
            AZStd::vector<RPI::ShaderOptionValuePair> qualityOptionValues = CreateEnumShaderOptionValues({"Quality::Low", "Quality::Medium", "Quality::High"});

            AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> shaderOptions = RPI::ShaderOptionGroupLayout::Create();
            shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_bool"}, ShaderOptionType::Boolean, 0, 0, boolOptionValues, Name{"False"}});
            shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_uint"}, ShaderOptionType::IntegerRange, 1, 1, intRangeOptionValues, Name{"0"}});
            shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_enum"}, ShaderOptionType::Enumeration, 5, 2, qualityOptionValues, Name{"Quality::Low"}});
            shaderOptions->Finalize();

            return shaderOptions;
        }
    };

    class TestMaterialData
    {
    public:
        // Setup for a single material property and nothing else, used in particular to test setting render states
        void Setup(
            MaterialPropertyDataType dataType,
            const char* materialPropertyName,
            const char* luaFunctorScript)
        {
            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(Uuid::CreateRandom());
            materialTypeCreator.AddShader(CreateTestShaderAsset(Uuid::CreateRandom()), AZ::RPI::ShaderVariantId{}, Name{"TestShader"});
            materialTypeCreator.BeginMaterialProperty(Name{materialPropertyName}, dataType);
            materialTypeCreator.EndMaterialProperty();
            LuaMaterialFunctorTests::AddLuaFunctor(materialTypeCreator, luaFunctorScript);
            EXPECT_TRUE(materialTypeCreator.End(m_materialTypeAsset));

            Data::Asset<MaterialAsset> materialAsset;
            MaterialAssetCreator materialCreator;
            materialCreator.Begin(Uuid::CreateRandom(), m_materialTypeAsset);
            EXPECT_TRUE(materialCreator.End(materialAsset));

            m_material = Material::Create(materialAsset);

            m_materialPropertyIndex = m_material->FindPropertyIndex(Name{materialPropertyName});
        }

        // Setup for a single material property and a specific shader constant input 
        void Setup(
            RHI::Ptr<RHI::ShaderResourceGroupLayout> materialSrgLayout,
            MaterialPropertyDataType dataType,
            const char* materialPropertyName,
            const char* shaderInputName,
            const char* luaFunctorScript)
        {
            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(Uuid::CreateRandom());
            materialTypeCreator.AddShader(CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout));
            materialTypeCreator.BeginMaterialProperty(Name{materialPropertyName}, dataType);
            materialTypeCreator.EndMaterialProperty();
            LuaMaterialFunctorTests::AddLuaFunctor(materialTypeCreator, luaFunctorScript);
            EXPECT_TRUE(materialTypeCreator.End(m_materialTypeAsset));

            Data::Asset<MaterialAsset> materialAsset;
            MaterialAssetCreator materialCreator;
            materialCreator.Begin(Uuid::CreateRandom(),m_materialTypeAsset);
            EXPECT_TRUE(materialCreator.End(materialAsset));

            m_material = Material::Create(materialAsset);

            m_materialPropertyIndex = m_material->FindPropertyIndex(Name{materialPropertyName});
            m_srgConstantIndex = m_material->GetRHIShaderResourceGroup()->GetData().FindShaderInputConstantIndex(Name{shaderInputName});
        }

        // Setup for a single material property and a specific shader option
        void Setup(
            AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> shaderOptionsLayout,
            MaterialPropertyDataType dataType,
            const char* materialPropertyName,
            const char* shaderOptionName,
            const char* luaFunctorScript)
        {
            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(Uuid::CreateRandom());
            materialTypeCreator.AddShader(CreateTestShaderAsset(Uuid::CreateRandom(), {}, shaderOptionsLayout), AZ::RPI::ShaderVariantId{}, Name{"TestShader"});
            materialTypeCreator.BeginMaterialProperty(Name{materialPropertyName}, dataType);
            materialTypeCreator.EndMaterialProperty();
            LuaMaterialFunctorTests::AddLuaFunctor(materialTypeCreator, luaFunctorScript);
            EXPECT_TRUE(materialTypeCreator.End(m_materialTypeAsset));

            Data::Asset<MaterialAsset> materialAsset;
            MaterialAssetCreator materialCreator;
            materialCreator.Begin(Uuid::CreateRandom(), m_materialTypeAsset);
            EXPECT_TRUE(materialCreator.End(materialAsset));

            m_material = Material::Create(materialAsset);

            m_materialPropertyIndex = m_material->FindPropertyIndex(Name{materialPropertyName});
            m_shaderOptionIndex = shaderOptionsLayout->FindShaderOptionIndex(Name{shaderOptionName});
        }

        // Setup for two material properties for testing one property affecting another property's metadata 
        void Setup(
            MaterialPropertyDataType primaryPropertyDataType,
            const char* primaryPropertyName,
            MaterialPropertyDataType secondaryPropertyDataType,
            const char* secondaryPropertyName,
            const char* luaFunctorScript)
        {
            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(Uuid::CreateRandom());
            materialTypeCreator.AddShader(CreateTestShaderAsset(Uuid::CreateRandom()));
            materialTypeCreator.BeginMaterialProperty(Name{primaryPropertyName}, primaryPropertyDataType);
            materialTypeCreator.EndMaterialProperty();
            materialTypeCreator.BeginMaterialProperty(Name{secondaryPropertyName}, secondaryPropertyDataType);
            materialTypeCreator.EndMaterialProperty();
            LuaMaterialFunctorTests::AddLuaFunctor(materialTypeCreator, luaFunctorScript);
            EXPECT_TRUE(materialTypeCreator.End(m_materialTypeAsset));

            Data::Asset<MaterialAsset> materialAsset;
            MaterialAssetCreator materialCreator;
            materialCreator.Begin(Uuid::CreateRandom(), m_materialTypeAsset);
            EXPECT_TRUE(materialCreator.End(materialAsset));

            m_material = Material::Create(materialAsset);

            m_materialPropertyIndex = m_material->FindPropertyIndex(Name{primaryPropertyName});
            m_otherMaterialPropertyIndex = m_material->FindPropertyIndex(Name{secondaryPropertyName});
        }

        // Setup for a single material property connected to a material pipeline property, with a material pipeline functor
        void SetupMaterialPipeline(
            MaterialPropertyDataType dataType,
            const char* materialPropertyName,
            const char* pipelineMaterialPropertyName,
            const char* luaFunctorScriptForMaterialPipeline)
        {
            Name materialPipelineName{"TestPipeline"};

            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(Uuid::CreateRandom());
            materialTypeCreator.AddShader(CreateTestShaderAsset(Uuid::CreateRandom()), AZ::RPI::ShaderVariantId{}, Name{"TestShader"}, materialPipelineName);
            materialTypeCreator.BeginMaterialProperty(Name{pipelineMaterialPropertyName}, dataType, materialPipelineName);
            materialTypeCreator.EndMaterialProperty();
            materialTypeCreator.BeginMaterialProperty(Name{materialPropertyName}, dataType);
            materialTypeCreator.ConnectMaterialPropertyToInternalProperty(Name{pipelineMaterialPropertyName});
            materialTypeCreator.EndMaterialProperty();
            LuaMaterialFunctorTests::AddLuaFunctor(materialTypeCreator, luaFunctorScriptForMaterialPipeline, materialPipelineName);
            EXPECT_TRUE(materialTypeCreator.End(m_materialTypeAsset));

            Data::Asset<MaterialAsset> materialAsset;
            MaterialAssetCreator materialCreator;
            materialCreator.Begin(Uuid::CreateRandom(), m_materialTypeAsset);
            EXPECT_TRUE(materialCreator.End(materialAsset));

            m_material = Material::Create(materialAsset);

            m_materialPropertyIndex = m_material->FindPropertyIndex(Name{materialPropertyName});
        }

        // Setup for a single material property connected to a material pipeline property, with a material pipeline functor, including a shader option
        void SetupMaterialPipeline(
            AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> shaderOptionsLayout,
            MaterialPropertyDataType dataType,
            const char* materialPropertyName,
            const char* pipelineMaterialPropertyName,
            const char* shaderOptionName,
            const char* luaFunctorScriptForMaterialPipeline)
        {
            Name materialPipelineName{"TestPipeline"};

            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(Uuid::CreateRandom());
            materialTypeCreator.AddShader(CreateTestShaderAsset(Uuid::CreateRandom(), {}, shaderOptionsLayout), AZ::RPI::ShaderVariantId{}, Name{"TestShader"}, materialPipelineName);
            materialTypeCreator.BeginMaterialProperty(Name{pipelineMaterialPropertyName}, dataType, materialPipelineName);
            materialTypeCreator.EndMaterialProperty();
            materialTypeCreator.BeginMaterialProperty(Name{materialPropertyName}, dataType);
            materialTypeCreator.ConnectMaterialPropertyToInternalProperty(Name{pipelineMaterialPropertyName});
            materialTypeCreator.EndMaterialProperty();
            LuaMaterialFunctorTests::AddLuaFunctor(materialTypeCreator, luaFunctorScriptForMaterialPipeline, materialPipelineName);
            EXPECT_TRUE(materialTypeCreator.End(m_materialTypeAsset));

            Data::Asset<MaterialAsset> materialAsset;
            MaterialAssetCreator materialCreator;
            materialCreator.Begin(Uuid::CreateRandom(), m_materialTypeAsset);
            EXPECT_TRUE(materialCreator.End(materialAsset));

            m_material = Material::Create(materialAsset);

            m_materialPropertyIndex = m_material->FindPropertyIndex(Name{materialPropertyName});
            m_shaderOptionIndex = shaderOptionsLayout->FindShaderOptionIndex(Name{shaderOptionName});
        }

        // Setup for a single material property with a material functor, and a material pipeline property with a material pipeline functor
        void SetupMaterialPipeline(
            MaterialPropertyDataType materialPropertyType,
            const char* materialPropertyName,
            const char* luaFunctorScript,
            MaterialPropertyDataType pipelineMaterialPropertyType,
            const char* pipelineMaterialPropertyName,
            const char* luaFunctorScriptForMaterialPipeline)
        {
            Name materialPipelineName{"TestPipeline"};

            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(Uuid::CreateRandom());
            materialTypeCreator.AddShader(CreateTestShaderAsset(Uuid::CreateRandom()), AZ::RPI::ShaderVariantId{}, Name{"TestShader"}, materialPipelineName);
            materialTypeCreator.BeginMaterialProperty(Name{pipelineMaterialPropertyName}, pipelineMaterialPropertyType, materialPipelineName);
            materialTypeCreator.EndMaterialProperty();
            materialTypeCreator.BeginMaterialProperty(Name{materialPropertyName}, materialPropertyType);
            materialTypeCreator.ConnectMaterialPropertyToInternalProperty(Name{pipelineMaterialPropertyName});
            materialTypeCreator.EndMaterialProperty();
            LuaMaterialFunctorTests::AddLuaFunctor(materialTypeCreator, luaFunctorScript);
            LuaMaterialFunctorTests::AddLuaFunctor(materialTypeCreator, luaFunctorScriptForMaterialPipeline, materialPipelineName);
            EXPECT_TRUE(materialTypeCreator.End(m_materialTypeAsset));

            Data::Asset<MaterialAsset> materialAsset;
            MaterialAssetCreator materialCreator;
            materialCreator.Begin(Uuid::CreateRandom(), m_materialTypeAsset);
            EXPECT_TRUE(materialCreator.End(materialAsset));

            m_material = Material::Create(materialAsset);

            m_materialPropertyIndex = m_material->FindPropertyIndex(Name{materialPropertyName});
        }

        Data::Asset<MaterialTypeAsset> GetMaterialTypeAsset() { return m_materialTypeAsset; }
        Data::Instance<Material> GetMaterial() { return m_material; }
        MaterialPropertyIndex GetMaterialPropertyIndex() { return m_materialPropertyIndex; }
        MaterialPropertyIndex GetOtherMaterialPropertyIndex() { return m_otherMaterialPropertyIndex; }
        RHI::ShaderInputConstantIndex GetSrgConstantIndex() { return m_srgConstantIndex; }
        ShaderOptionIndex GetShaderOptionIndex() { return m_shaderOptionIndex; }

    private:
        Data::Asset<MaterialTypeAsset> m_materialTypeAsset;
        Data::Instance<Material> m_material;
        MaterialPropertyIndex m_materialPropertyIndex;
        MaterialPropertyIndex m_otherMaterialPropertyIndex;
        RHI::ShaderInputConstantIndex m_srgConstantIndex;
        ShaderOptionIndex m_shaderOptionIndex;
    };

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetShaderConstant_Bool)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestBool"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_bool("general.TestBool")
                    context:SetShaderConstant_bool("m_bool", value)
                end
            )";

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        TestMaterialData testData;
        testData.Setup(materialSrgLayout, MaterialPropertyDataType::Bool, "general.TestBool", "m_bool", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(true, testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<bool>(testData.GetSrgConstantIndex()));

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{false});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(false, testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<bool>(testData.GetSrgConstantIndex()));
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetShaderConstant_Float)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestFloat"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_float("general.TestFloat")
                    context:SetShaderConstant_float("m_float", value * 2.0)
                end
            )";

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        TestMaterialData testData;
        testData.Setup(materialSrgLayout, MaterialPropertyDataType::Float, "general.TestFloat", "m_float", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{1.25f});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_FLOAT_EQ(2.5f, testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<float>(testData.GetSrgConstantIndex()));
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetShaderConstant_Int)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestInt"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_int("general.TestInt")
                    context:SetShaderConstant_int("m_int", value * -1)
                end
            )";

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        TestMaterialData testData;
        testData.Setup(materialSrgLayout, MaterialPropertyDataType::Int, "general.TestInt", "m_int", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{2});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(-2, testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<int32_t>(testData.GetSrgConstantIndex()));
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetShaderConstant_UInt)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestUInt"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_uint("general.TestUInt")
                    context:SetShaderConstant_uint("m_uint", value + 5)
                end
            )";

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        TestMaterialData testData;
        testData.Setup(materialSrgLayout, MaterialPropertyDataType::UInt, "general.TestUInt", "m_uint", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{2u});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(7, testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<uint32_t>(testData.GetSrgConstantIndex()));
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetShaderConstant_Float2)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestVector2"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_Vector2("general.TestVector2")
                    local swap = value.y
                    value.y = value.x
                    value.x = swap
                    context:SetShaderConstant_Vector2("m_float2", value)
                end
            )";

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        TestMaterialData testData;
        testData.Setup(materialSrgLayout, MaterialPropertyDataType::Vector2, "general.TestVector2", "m_float2", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{Vector2(1.0f, 2.0f)});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(Vector2(2.0f, 1.0f), testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<Vector2>(testData.GetSrgConstantIndex()));
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetShaderConstant_Vector3)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestVector3"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_Vector3("general.TestVector3")
                    value:Normalize()
                    context:SetShaderConstant_Vector3("m_float3", value)
                end
            )";

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        TestMaterialData testData;
        testData.Setup(materialSrgLayout, MaterialPropertyDataType::Vector3, "general.TestVector3", "m_float3", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{Vector3(5.0f, 4.0f, 3.0f)});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(Vector3(5.0f, 4.0f, 3.0f).GetNormalized(), testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<Vector3>(testData.GetSrgConstantIndex()));
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetShaderConstant_Vector4)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestVector4"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_Vector4("general.TestVector4")
                    value:Homogenize()
                    context:SetShaderConstant_Vector4("m_float4", value)
                end
            )";

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        TestMaterialData testData;
        testData.Setup(materialSrgLayout, MaterialPropertyDataType::Vector4, "general.TestVector4", "m_float4", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{Vector4(1.0f, 2.0f, 3.0f, 4.0f)});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(Vector4(1.0f, 2.0f, 3.0f, 4.0f) / 4.0f, testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<Vector4>(testData.GetSrgConstantIndex()));
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetShaderConstant_Color)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestColor"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_Color("general.TestColor")
                    value.r = value.r * value.a
                    value.g = value.g * value.a
                    value.b = value.b * value.a
                    context:SetShaderConstant_Color("m_color", value)
                end
            )";

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        TestMaterialData testData;
        testData.Setup(materialSrgLayout, MaterialPropertyDataType::Color, "general.TestColor", "m_color", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{Color(1.0f, 0.5f, 0.4f, 0.5f)});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(Color(0.5f, 0.25f, 0.2f, 0.5f), testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<Color>(testData.GetSrgConstantIndex()));
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_SetShaderConstant_Matrix3x3)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.Scale"}
                end

                function Process(context)
                    local scale = context:GetMaterialPropertyValue_float("general.Scale")
                    local tansform = Matrix3x3.CreateScale(Vector3(scale, scale, 1.0))
                    context:SetShaderConstant_Matrix3x3("m_float3x3", tansform)
                end
            )";

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        TestMaterialData testData;
        testData.Setup(materialSrgLayout, MaterialPropertyDataType::Float, "general.Scale", "m_float3x3", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{0.5f});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(Matrix3x3::CreateScale(Vector3(0.5f, 0.5f, 1.0f)), testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<Matrix3x3>(testData.GetSrgConstantIndex()));
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_SetShaderConstant_Matrix4x4)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.Offset"}
                end

                function Process(context)
                    local offset = context:GetMaterialPropertyValue_Vector3("general.Offset")
                    local tansform = Matrix4x4.CreateTranslation(offset)
                    context:SetShaderConstant_Matrix4x4("m_float4x4", tansform)
                end
            )";

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        TestMaterialData testData;
        testData.Setup(materialSrgLayout, MaterialPropertyDataType::Vector3, "general.Offset", "m_float4x4", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{Vector3(1.0f, 2.0f, 3.0f)});
        ProcessQueuedSrgCompilations(shaderAsset, materialSrgLayout->GetName());
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(Matrix4x4::CreateTranslation(Vector3(1.0f, 2.0f, 3.0f)), testData.GetMaterial()->GetRHIShaderResourceGroup()->GetData().GetConstant<Matrix4x4>(testData.GetSrgConstantIndex()));
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_SetShaderOption_Bool)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestBool"}
                end

                function GetShaderOptionDependencies()
                    return {"o_bool"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_bool("general.TestBool")
                    context:SetShaderOptionValue_bool("o_bool", value)
                end
            )";

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> options = CreateCommonTestShaderOptionsLayout();

        TestMaterialData testData;
        testData.Setup(options, MaterialPropertyDataType::Bool, "general.TestBool", "o_bool", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(1, testData.GetMaterial()->GetGeneralShaderCollection()[0].GetShaderOptions()->GetValue(Name{"o_bool"}).GetIndex());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{false});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(0, testData.GetMaterial()->GetGeneralShaderCollection()[0].GetShaderOptions()->GetValue(Name{"o_bool"}).GetIndex());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_SetShaderOption_UInt)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestInt"}
                end

                function GetShaderOptionDependencies()
                    return {"o_uint"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_int("general.TestInt")
                    context:SetShaderOptionValue_uint("o_uint", value * 2)
                end
            )";

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> options = CreateCommonTestShaderOptionsLayout();

        TestMaterialData testData;
        testData.Setup(options, MaterialPropertyDataType::Int, "general.TestInt", "o_uint", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{6});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(12, testData.GetMaterial()->GetGeneralShaderCollection()[0].GetShaderOptions()->GetValue(Name{"o_uint"}).GetIndex());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_SetShaderOption_Enum)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.UseHighQuality"}
                end

                function GetShaderOptionDependencies()
                    return {"o_enum"}
                end

                function Process(context)
                    local useHighQuality = context:GetMaterialPropertyValue_bool("general.UseHighQuality")
                    if (useHighQuality) then
                        context:SetShaderOptionValue_enum("o_enum", "Quality::High")
                    else
                        context:SetShaderOptionValue_enum("o_enum", "Quality::Medium")
                    end
                end
            )";

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> options = CreateCommonTestShaderOptionsLayout();

        TestMaterialData testData;
        testData.Setup(options, MaterialPropertyDataType::Bool, "general.UseHighQuality", "o_enum", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(2, testData.GetMaterial()->GetGeneralShaderCollection()[0].GetShaderOptions()->GetValue(Name{"o_enum"}).GetIndex());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{false});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(1, testData.GetMaterial()->GetGeneralShaderCollection()[0].GetShaderOptions()->GetValue(Name{"o_enum"}).GetIndex());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_ShaderItem_SetShaderOption_Bool)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestBool"}
                end

                function GetShaderOptionDependencies()
                    return {"o_bool"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_bool("general.TestBool")
                    context:GetShaderByTag("TestShader"):SetShaderOptionValue_bool("o_bool", value)
                end
            )";

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> options = CreateCommonTestShaderOptionsLayout();

        TestMaterialData testData;
        testData.Setup(options, MaterialPropertyDataType::Bool, "general.TestBool", "o_bool", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(
            1, testData.GetMaterial()->GetGeneralShaderCollection()[Name{"TestShader"}].GetShaderOptions()->GetValue(Name{"o_bool"}).GetIndex());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{false});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(
            0, testData.GetMaterial()->GetGeneralShaderCollection()[Name{"TestShader"}].GetShaderOptions()->GetValue(Name{"o_bool"}).GetIndex());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_ShaderItem_SetShaderOption_UInt)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestInt"}
                end

                function GetShaderOptionDependencies()
                    return {"o_uint"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_int("general.TestInt")
                    context:GetShaderByTag("TestShader"):SetShaderOptionValue_uint("o_uint", value * 2)
                end
            )";

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> options = CreateCommonTestShaderOptionsLayout();

        TestMaterialData testData;
        testData.Setup(options, MaterialPropertyDataType::Int, "general.TestInt", "o_uint", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{6});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(
            12, testData.GetMaterial()->GetGeneralShaderCollection()[Name{"TestShader"}].GetShaderOptions()->GetValue(Name{"o_uint"}).GetIndex());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_ShaderItem_SetShaderOption_Enum)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.UseHighQuality"}
                end

                function GetShaderOptionDependencies()
                    return {"o_enum"}
                end

                function Process(context)
                    local useHighQuality = context:GetMaterialPropertyValue_bool("general.UseHighQuality")
                    if (useHighQuality) then
                        context:GetShaderByTag("TestShader"):SetShaderOptionValue_enum("o_enum", "Quality::High")
                    else
                        context:GetShaderByTag("TestShader"):SetShaderOptionValue_enum("o_enum", "Quality::Medium")
                    end
                end
            )";

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> options = CreateCommonTestShaderOptionsLayout();

        TestMaterialData testData;
        testData.Setup(options, MaterialPropertyDataType::Bool, "general.UseHighQuality", "o_enum", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(
            2, testData.GetMaterial()->GetGeneralShaderCollection()[Name{"TestShader"}].GetShaderOptions()->GetValue(Name{"o_enum"}).GetIndex());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{false});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(
            1, testData.GetMaterial()->GetGeneralShaderCollection()[Name{"TestShader"}].GetShaderOptions()->GetValue(Name{"o_enum"}).GetIndex());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_EditorContext_SetMaterialPropertyVisibility)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return { "general.mode" }
                end

                function ProcessEditor(context)
                    local mode = context:GetMaterialPropertyValue_uint("general.mode")

                    if (mode == 1) then
                        context:SetMaterialPropertyVisibility("general.value", MaterialPropertyVisibility_Enabled)
                    elseif (mode == 2) then
                        context:SetMaterialPropertyVisibility("general.value", MaterialPropertyVisibility_Disabled)
                    else
                        context:SetMaterialPropertyVisibility("general.value", MaterialPropertyVisibility_Hidden)
                    end
                end
            )";

        TestMaterialData testData;
        testData.Setup(
            MaterialPropertyDataType::UInt, "general.mode",
            MaterialPropertyDataType::Float, "general.value",
            functorScript);
        
        AZStd::unordered_set<AZ::Name> changedPropertyNames;
        AZStd::unordered_map<Name, MaterialPropertyDynamicMetadata> propertyDynamicMetadata;
        propertyDynamicMetadata[Name{"general.mode"}] = {};
        propertyDynamicMetadata[Name{"general.value"}] = {};
        
        AZStd::unordered_set<AZ::Name> changedPropertyGroupNames;
        AZStd::unordered_map<Name, MaterialPropertyGroupDynamicMetadata> propertyGroupDynamicMetadata;
        propertyGroupDynamicMetadata[Name{"general"}] = {};

        Ptr<MaterialFunctor> functor = testData.GetMaterialTypeAsset()->GetMaterialFunctors()[0];

        AZ::RPI::MaterialFunctorAPI::EditorContext context = AZ::RPI::MaterialFunctorAPI::EditorContext(
            testData.GetMaterial()->GetPropertyCollection(),
            propertyDynamicMetadata,
            propertyGroupDynamicMetadata,
            changedPropertyNames,
            changedPropertyGroupNames,
            &functor->GetMaterialPropertyDependencies()
        );

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{0u});
        functor->Process(context);
        EXPECT_EQ(MaterialPropertyVisibility::Hidden, propertyDynamicMetadata[Name{"general.value"}].m_visibility);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{1u});
        functor->Process(context);
        EXPECT_EQ(MaterialPropertyVisibility::Enabled, propertyDynamicMetadata[Name{"general.value"}].m_visibility);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{2u});
        functor->Process(context);
        EXPECT_EQ(MaterialPropertyVisibility::Disabled, propertyDynamicMetadata[Name{"general.value"}].m_visibility);
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_EditorContext_SetMaterialPropertyDescriptionAndRanges)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return { "general.units" }
                end

                function ProcessEditor(context)
                    local units = context:GetMaterialPropertyValue_uint("general.units")

                    if (units == 0) then
                        context:SetMaterialPropertyDescription("general.distance", "Distance in meters")
                        context:SetMaterialPropertyMinValue_float("general.distance", -10)
                        context:SetMaterialPropertyMaxValue_float("general.distance",  10)
                        context:SetMaterialPropertySoftMinValue_float("general.distance", -1)
                        context:SetMaterialPropertySoftMaxValue_float("general.distance",  1)
                    else
                        context:SetMaterialPropertyDescription("general.distance", "Distance in centimeters")
                        context:SetMaterialPropertyMinValue_float("general.distance", -1000)
                        context:SetMaterialPropertyMaxValue_float("general.distance",  1000)
                        context:SetMaterialPropertySoftMinValue_float("general.distance", -100)
                        context:SetMaterialPropertySoftMaxValue_float("general.distance",  100)
                    end
                end
            )";

        TestMaterialData testData;
        testData.Setup(
            MaterialPropertyDataType::UInt, "general.units",
            MaterialPropertyDataType::Float, "general.distance",
            functorScript);

        AZStd::unordered_set<AZ::Name> changedPropertyNames;
        AZStd::unordered_map<Name, MaterialPropertyDynamicMetadata> propertyDynamicMetadata;
        propertyDynamicMetadata[Name{"general.units"}] = {};
        propertyDynamicMetadata[Name{"general.distance"}] = {};
        
        AZStd::unordered_set<AZ::Name> changedPropertyGroupNames;
        AZStd::unordered_map<Name, MaterialPropertyGroupDynamicMetadata> propertyGroupDynamicMetadata;
        propertyGroupDynamicMetadata[Name{"general"}] = {};

        Ptr<MaterialFunctor> functor = testData.GetMaterialTypeAsset()->GetMaterialFunctors()[0];

        AZ::RPI::MaterialFunctorAPI::EditorContext context = AZ::RPI::MaterialFunctorAPI::EditorContext(
            testData.GetMaterial()->GetPropertyCollection(),
            propertyDynamicMetadata,
            propertyGroupDynamicMetadata,
            changedPropertyNames,
            changedPropertyGroupNames,
            &functor->GetMaterialPropertyDependencies()
        );

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{0u});
        functor->Process(context);
        EXPECT_STREQ("Distance in meters", propertyDynamicMetadata[Name{"general.distance"}].m_description.c_str());
        EXPECT_EQ(-10.0f, propertyDynamicMetadata[Name{"general.distance"}].m_propertyRange.m_min.GetValue<float>());
        EXPECT_EQ(10.0f, propertyDynamicMetadata[Name{"general.distance"}].m_propertyRange.m_max.GetValue<float>());
        EXPECT_EQ(-1.0f, propertyDynamicMetadata[Name{"general.distance"}].m_propertyRange.m_softMin.GetValue<float>());
        EXPECT_EQ(1.0f, propertyDynamicMetadata[Name{"general.distance"}].m_propertyRange.m_softMax.GetValue<float>());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{1u});
        functor->Process(context);
        EXPECT_STREQ("Distance in centimeters", propertyDynamicMetadata[Name{"general.distance"}].m_description.c_str());
        EXPECT_EQ(-1000.0f, propertyDynamicMetadata[Name{"general.distance"}].m_propertyRange.m_min.GetValue<float>());
        EXPECT_EQ(1000.0f, propertyDynamicMetadata[Name{"general.distance"}].m_propertyRange.m_max.GetValue<float>());
        EXPECT_EQ(-100.0f, propertyDynamicMetadata[Name{"general.distance"}].m_propertyRange.m_softMin.GetValue<float>());
        EXPECT_EQ(100.0f, propertyDynamicMetadata[Name{"general.distance"}].m_propertyRange.m_softMax.GetValue<float>());
    }
    
    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_EditorContext_SetMaterialPropertyGroupVisibility)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return { "general.mode" }
                end

                function ProcessEditor(context)
                    local mode = context:GetMaterialPropertyValue_uint("general.mode")

                    if (mode == 1) then
                        context:SetMaterialPropertyGroupVisibility("otherGroup", MaterialPropertyGroupVisibility_Enabled)
                    else
                        context:SetMaterialPropertyGroupVisibility("otherGroup", MaterialPropertyGroupVisibility_Hidden)
                    end
                end
            )";

        TestMaterialData testData;
        testData.Setup(
            MaterialPropertyDataType::UInt, "general.mode",
            MaterialPropertyDataType::Float, "otherGroup.value",
            functorScript);
        
        AZStd::unordered_set<AZ::Name> changedPropertyNames;
        AZStd::unordered_map<Name, MaterialPropertyDynamicMetadata> propertyDynamicMetadata;
        propertyDynamicMetadata[Name{"general.mode"}] = {};
        propertyDynamicMetadata[Name{"otherGroup.value"}] = {};
        
        AZStd::unordered_set<AZ::Name> changedPropertyGroupNames;
        AZStd::unordered_map<Name, MaterialPropertyGroupDynamicMetadata> propertyGroupDynamicMetadata;
        propertyGroupDynamicMetadata[Name{"general"}] = {};
        propertyGroupDynamicMetadata[Name{"otherGroup"}] = {};

        Ptr<MaterialFunctor> functor = testData.GetMaterialTypeAsset()->GetMaterialFunctors()[0];

        AZ::RPI::MaterialFunctorAPI::EditorContext context = AZ::RPI::MaterialFunctorAPI::EditorContext(
            testData.GetMaterial()->GetPropertyCollection(),
            propertyDynamicMetadata,
            propertyGroupDynamicMetadata,
            changedPropertyNames,
            changedPropertyGroupNames,
            &functor->GetMaterialPropertyDependencies()
        );

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{0u});
        functor->Process(context);
        EXPECT_EQ(MaterialPropertyGroupVisibility::Enabled, propertyGroupDynamicMetadata[Name{"general"}].m_visibility);
        EXPECT_EQ(MaterialPropertyGroupVisibility::Hidden, propertyGroupDynamicMetadata[Name{"otherGroup"}].m_visibility);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{1u});
        functor->Process(context);
        EXPECT_EQ(MaterialPropertyGroupVisibility::Enabled, propertyGroupDynamicMetadata[Name{"general"}].m_visibility);
        EXPECT_EQ(MaterialPropertyGroupVisibility::Enabled, propertyGroupDynamicMetadata[Name{"otherGroup"}].m_visibility);
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_SetRenderStates)
    {
        using namespace AZ::RPI;

        // We aren't testing every single render state here, but just getting a representative set...

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.MyBool"}
                end

                function Process(context)
                    local boolValue = context:GetMaterialPropertyValue_bool("general.MyBool")
                    if(boolValue) then
                        context:GetShader(0):GetRenderStatesOverride():SetMultisampleCustomPositionCount(1)
                        context:GetShader(0):GetRenderStatesOverride():SetMultisampleCustomPosition(0, 2, 4)
                        context:GetShader(0):GetRenderStatesOverride():SetCullMode(CullMode_None)
                        context:GetShader(0):GetRenderStatesOverride():SetBlendEnabled(1, true)
                        context:GetShader(0):GetRenderStatesOverride():SetDepthBias(-1)
                        context:GetShader(0):GetRenderStatesOverride():SetDepthBiasClamp(0.2)
                        context:GetShader(0):GetRenderStatesOverride():SetStencilWriteMask(0xF0)
                    else
                        context:GetShader(0):GetRenderStatesOverride():ClearMultisampleCustomPositionCount()
                        context:GetShader(0):GetRenderStatesOverride():ClearMultisampleCustomPosition(0)
                        context:GetShader(0):GetRenderStatesOverride():ClearCullMode()
                        context:GetShader(0):GetRenderStatesOverride():ClearBlendEnabled(1)
                        context:GetShader(0):GetRenderStatesOverride():ClearDepthBias()
                        context:GetShader(0):GetRenderStatesOverride():ClearDepthBiasClamp()
                        context:GetShader(0):GetRenderStatesOverride():ClearStencilWriteMask()
                    end
                end
            )";

        TestMaterialData testData;
        testData.Setup(MaterialPropertyDataType::Bool, "general.MyBool", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());

        const ShaderCollection& shaderCollection = testData.GetMaterial()->GetGeneralShaderCollection();

        EXPECT_EQ(1, shaderCollection[0].GetRenderStatesOverlay()->m_multisampleState.m_customPositionsCount);
        EXPECT_EQ(2, shaderCollection[0].GetRenderStatesOverlay()->m_multisampleState.m_customPositions[0].m_x);
        EXPECT_EQ(4, shaderCollection[0].GetRenderStatesOverlay()->m_multisampleState.m_customPositions[0].m_y);
        EXPECT_EQ(RHI::CullMode::None, shaderCollection[0].GetRenderStatesOverlay()->m_rasterState.m_cullMode);
        EXPECT_EQ(1, shaderCollection[0].GetRenderStatesOverlay()->m_blendState.m_targets[1].m_enable);
        EXPECT_EQ(-1, shaderCollection[0].GetRenderStatesOverlay()->m_rasterState.m_depthBias);
        EXPECT_FLOAT_EQ(0.2, shaderCollection[0].GetRenderStatesOverlay()->m_rasterState.m_depthBiasClamp);
        EXPECT_EQ(0xF0, shaderCollection[0].GetRenderStatesOverlay()->m_depthStencilState.m_stencil.m_writeMask);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{false});
        EXPECT_TRUE(testData.GetMaterial()->Compile());

        EXPECT_EQ(RHI::RenderStates_InvalidUInt, shaderCollection[0].GetRenderStatesOverlay()->m_multisampleState.m_customPositionsCount);
        EXPECT_EQ(RHI::Limits::Pipeline::MultiSampleCustomLocationGridSize, shaderCollection[0].GetRenderStatesOverlay()->m_multisampleState.m_customPositions[0].m_x);
        EXPECT_EQ(RHI::Limits::Pipeline::MultiSampleCustomLocationGridSize, shaderCollection[0].GetRenderStatesOverlay()->m_multisampleState.m_customPositions[0].m_y);
        EXPECT_EQ(RHI::CullMode::Invalid, shaderCollection[0].GetRenderStatesOverlay()->m_rasterState.m_cullMode);
        EXPECT_EQ(RHI::RenderStates_InvalidBool, shaderCollection[0].GetRenderStatesOverlay()->m_blendState.m_targets[1].m_enable);
        EXPECT_EQ(RHI::RenderStates_InvalidInt, shaderCollection[0].GetRenderStatesOverlay()->m_rasterState.m_depthBias);
        EXPECT_FLOAT_EQ(RHI::RenderStates_InvalidFloat, shaderCollection[0].GetRenderStatesOverlay()->m_rasterState.m_depthBiasClamp);
        EXPECT_EQ(RHI::RenderStates_InvalidUInt, shaderCollection[0].GetRenderStatesOverlay()->m_depthStencilState.m_stencil.m_writeMask);
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_SetShaderEnabledByTag)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.MyBool"}
                end

                function Process(context)
                    local boolValue = context:GetMaterialPropertyValue_bool("general.MyBool")
                    context:GetShaderByTag("TestShader"):SetEnabled(boolValue)
                end
            )";

        TestMaterialData testData;
        testData.Setup(MaterialPropertyDataType::Bool, "general.MyBool", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());

        EXPECT_EQ(true, testData.GetMaterial()->GetGeneralShaderCollection()[Name{"TestShader"}].IsEnabled());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_SetShaderDrawListTagOverride)
    {
        using namespace AZ::RPI;

        RHI::DrawListTagRegistry* drawListTagRegistry = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry();
        drawListTagRegistry->AcquireTag(Name{"TestDrawListTag"});

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.MyBool"}
                end

                function Process(context)
                    context:GetShaderByTag("TestShader"):SetDrawListTagOverride("TestDrawListTag")
                end
            )";

        TestMaterialData testData;
        testData.Setup(MaterialPropertyDataType::Bool, "general.MyBool", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());

        RHI::DrawListTag tag = drawListTagRegistry->FindTag(Name{"TestDrawListTag"});
        EXPECT_EQ(tag, testData.GetMaterial()->GetGeneralShaderCollection()[Name{"TestShader"}].GetDrawListTagOverride());

        drawListTagRegistry->ReleaseTag(tag);
    }
    
    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_PsoChangesNotAllowed_Error)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.MyBool"}
                end

                function GetShaderOptionDependencies()
                    return {}
                end

                function Process(context)
                    local boolValue = context:GetMaterialPropertyValue_bool("general.MyBool")
                    if(boolValue) then
                        context:GetShader(0):GetRenderStatesOverride():SetFillMode(FillMode_Wireframe)
                    else
                        context:GetShader(0):GetRenderStatesOverride():ClearFillMode()
                    end
                end
            )";

        TestMaterialData testData;
        testData.Setup(MaterialPropertyDataType::Bool, "general.MyBool", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});

        ErrorMessageFinder errorMessageFinder;

        errorMessageFinder.AddExpectedErrorMessage("not be changed at runtime because they impact Pipeline State Objects: general.MyBool");
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_MultisampleCustomPositionCountIndex_Error)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.MyBool"}
                end

                function GetShaderOptionDependencies()
                    return {}
                end

                function Process(context)
                    local boolValue = context:GetMaterialPropertyValue_bool("general.MyBool")
                    if(boolValue) then
                        context:GetShader(0):GetRenderStatesOverride():SetMultisampleCustomPositionCount(20)
                    else
                        context:GetShader(0):GetRenderStatesOverride():ClearMultisampleCustomPositionCount()
                    end
                end
            )";

        TestMaterialData testData;
        testData.Setup(MaterialPropertyDataType::Bool, "general.MyBool", functorScript);

        testData.GetMaterial()->SetPsoHandlingOverride(AZ::RPI::MaterialPropertyPsoHandling::Allowed);
        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});

        ErrorMessageFinder errorMessageFinder;

        errorMessageFinder.AddExpectedErrorMessage("SetMultisampleCustomPositionCount(20) value is out of range. Must be less than 16.");
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_MultisampleCustomPositionIndex_Error)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.MyBool"}
                end

                function GetShaderOptionDependencies()
                    return {}
                end

                function Process(context)
                    local boolValue = context:GetMaterialPropertyValue_bool("general.MyBool")
                    if(boolValue) then
                        context:GetShader(0):GetRenderStatesOverride():SetMultisampleCustomPosition(17, 0, 0)
                    else
                        context:GetShader(0):GetRenderStatesOverride():ClearMultisampleCustomPosition(18)
                    end
                end
            )";

        TestMaterialData testData;

        ErrorMessageFinder errorMessageFinder;

        errorMessageFinder.AddExpectedErrorMessage("ClearMultisampleCustomPosition(18,...) index is out of range. Must be less than 16.");
        testData.Setup(MaterialPropertyDataType::Bool, "general.MyBool", functorScript);
        errorMessageFinder.CheckExpectedErrorsFound();
        
        testData.GetMaterial()->SetPsoHandlingOverride(AZ::RPI::MaterialPropertyPsoHandling::Allowed);
        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});

        errorMessageFinder.AddExpectedErrorMessage("SetMultisampleCustomPosition(17,...) index is out of range. Must be less than 16.");
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_BlendStateIndex_Error)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.MyBool"}
                end

                function GetShaderOptionDependencies()
                    return {}
                end

                function Process(context)
                    local boolValue = context:GetMaterialPropertyValue_bool("general.MyBool")
                    if(boolValue) then
                        context:GetShader(0):GetRenderStatesOverride():SetBlendEnabled(9, false)
                    else
                        context:GetShader(0):GetRenderStatesOverride():ClearBlendEnabled(10)
                    end
                end
            )";

        TestMaterialData testData;

        ErrorMessageFinder errorMessageFinder;

        errorMessageFinder.AddExpectedErrorMessage("ClearBlendEnabled(10,...) index is out of range. Must be less than 8.");
        testData.Setup(MaterialPropertyDataType::Bool, "general.MyBool", functorScript);
        errorMessageFinder.CheckExpectedErrorsFound();
        
        testData.GetMaterial()->SetPsoHandlingOverride(AZ::RPI::MaterialPropertyPsoHandling::Allowed);
        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});

        errorMessageFinder.AddExpectedErrorMessage("SetBlendEnabled(9,...) index is out of range. Must be less than 8.");
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_PipelineRuntimeContext_GetMaterialProperty_SetShaderEnabled_Bool)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"EnableTestShader"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_bool("EnableTestShader")
                    context:GetShaderByTag("TestShader"):SetEnabled(value)
                end
            )";

        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), nullptr);

        TestMaterialData testData;
        testData.SetupMaterialPipeline(MaterialPropertyDataType::Bool, "general.TestBool", "EnableTestShader", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_TRUE(testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[Name{"TestShader"}].IsEnabled());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{false});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_FALSE(testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[Name{"TestShader"}].IsEnabled());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_PipelineRuntimeContext_SetShaderOption_Bool)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"TestPipelineProperty"}
                end

                function GetShaderOptionDependencies()
                    return {"o_bool"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_bool("TestPipelineProperty")
                    context:SetShaderOptionValue_bool("o_bool", value)
                end
            )";

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> options = CreateCommonTestShaderOptionsLayout();

        TestMaterialData testData;
        testData.SetupMaterialPipeline(options, MaterialPropertyDataType::Bool, "general.TestBool", "TestPipelineProperty", "o_bool", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(1, testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[0].GetShaderOptions()->GetValue(Name{"o_bool"}).GetIndex());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{false});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(0, testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[0].GetShaderOptions()->GetValue(Name{"o_bool"}).GetIndex());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_PipelineRuntimeContext_SetShaderOption_UInt)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"TestPipelineProperty"}
                end

                function GetShaderOptionDependencies()
                    return {"o_uint"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_int("TestPipelineProperty")
                    context:SetShaderOptionValue_uint("o_uint", value * 2)
                end
            )";

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> options = CreateCommonTestShaderOptionsLayout();

        TestMaterialData testData;
        testData.SetupMaterialPipeline(options, MaterialPropertyDataType::Int, "general.TestInt", "TestPipelineProperty", "o_uint", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{6});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(12, testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[0].GetShaderOptions()->GetValue(Name{"o_uint"}).GetIndex());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_PipelineRuntimeContext_SetShaderOption_Enum)
    {
        using namespace AZ::RPI;

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"TestPipelineProperty"}
                end

                function GetShaderOptionDependencies()
                    return {"o_enum"}
                end

                function Process(context)
                    local useHighQuality = context:GetMaterialPropertyValue_bool("TestPipelineProperty")
                    if (useHighQuality) then
                        context:SetShaderOptionValue_enum("o_enum", "Quality::High")
                    else
                        context:SetShaderOptionValue_enum("o_enum", "Quality::Medium")
                    end
                end
            )";

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> options = CreateCommonTestShaderOptionsLayout();

        TestMaterialData testData;
        testData.SetupMaterialPipeline(options, MaterialPropertyDataType::Bool, "general.UseHighQuality", "TestPipelineProperty", "o_enum", functorScript);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(2, testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[0].GetShaderOptions()->GetValue(Name{"o_enum"}).GetIndex());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{false});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_EQ(1, testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[0].GetShaderOptions()->GetValue(Name{"o_enum"}).GetIndex());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_SetInternalProperty_Bool)
    {
        using namespace AZ::RPI;

        // One functor passes a property value to an internal property of the material pipeline,
        // then a material pipeline functor uses that value to enable or disable a shader.

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestProperty"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_bool("general.TestProperty")
                    context:SetInternalMaterialPropertyValue_bool("TestPipelineProperty", value)
                end
            )";

        const char* functorScriptForPipeline =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"TestPipelineProperty"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_bool("TestPipelineProperty")
                    context:GetShaderByTag("TestShader"):SetEnabled(value)
                end
            )";

        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), nullptr);

        TestMaterialData testData;
        testData.SetupMaterialPipeline(
            MaterialPropertyDataType::Bool, "general.TestProperty", functorScript,
            MaterialPropertyDataType::Bool, "TestPipelineProperty", functorScriptForPipeline);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{true});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_TRUE(testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[Name{"TestShader"}].IsEnabled());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{false});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_FALSE(testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[Name{"TestShader"}].IsEnabled());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetInternalProperty_Float)
    {
        using namespace AZ::RPI;

        // One functor passes a property value to an internal property of the material pipeline,
        // then a material pipeline functor uses that value to enable or disable a shader.

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestProperty"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_float("general.TestProperty")
                    context:SetInternalMaterialPropertyValue_float("TestPipelineProperty", value)
                end
            )";

        const char* functorScriptForPipeline =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"TestPipelineProperty"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_float("TestPipelineProperty")
                    context:GetShaderByTag("TestShader"):SetEnabled(value > 0.0)
                end
            )";

        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), nullptr);

        TestMaterialData testData;
        testData.SetupMaterialPipeline(
            MaterialPropertyDataType::Float, "general.TestProperty", functorScript,
            MaterialPropertyDataType::Float, "TestPipelineProperty", functorScriptForPipeline);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{1.0f});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_TRUE(testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[Name{"TestShader"}].IsEnabled());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{-1.0f});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_FALSE(testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[Name{"TestShader"}].IsEnabled());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetInternalProperty_Int)
    {
        using namespace AZ::RPI;

        // One functor passes a property value to an internal property of the material pipeline,
        // then a material pipeline functor uses that value to enable or disable a shader.

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestProperty"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_int("general.TestProperty")
                    context:SetInternalMaterialPropertyValue_int("TestPipelineProperty", value)
                end
            )";

        const char* functorScriptForPipeline =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"TestPipelineProperty"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_int("TestPipelineProperty")
                    context:GetShaderByTag("TestShader"):SetEnabled(value ~= 0)
                end
            )";

        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), nullptr);

        TestMaterialData testData;
        testData.SetupMaterialPipeline(
            MaterialPropertyDataType::Int, "general.TestProperty", functorScript,
            MaterialPropertyDataType::Int, "TestPipelineProperty", functorScriptForPipeline);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{-1});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_TRUE(testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[Name{"TestShader"}].IsEnabled());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{0});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_FALSE(testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[Name{"TestShader"}].IsEnabled());
    }

    TEST_F(LuaMaterialFunctorTests, LuaMaterialFunctor_RuntimeContext_GetMaterialProperty_SetInternalProperty_UInt)
    {
        using namespace AZ::RPI;

        // One functor passes a property value to an internal property of the material pipeline,
        // then a material pipeline functor uses that value to enable or disable a shader.

        const char* functorScript =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"general.TestProperty"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_uint("general.TestProperty")
                    context:SetInternalMaterialPropertyValue_uint("TestPipelineProperty", value)
                end
            )";

        const char* functorScriptForPipeline =
            R"(
                function GetMaterialPropertyDependencies()
                    return {"TestPipelineProperty"}
                end

                function Process(context)
                    local value = context:GetMaterialPropertyValue_uint("TestPipelineProperty")
                    context:GetShaderByTag("TestShader"):SetEnabled(value ~= 0)
                end
            )";

        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), nullptr);

        TestMaterialData testData;
        testData.SetupMaterialPipeline(
            MaterialPropertyDataType::UInt, "general.TestProperty", functorScript,
            MaterialPropertyDataType::UInt, "TestPipelineProperty", functorScriptForPipeline);

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{1u});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_TRUE(testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[Name{"TestShader"}].IsEnabled());

        testData.GetMaterial()->SetPropertyValue(testData.GetMaterialPropertyIndex(), MaterialPropertyValue{0u});
        EXPECT_TRUE(testData.GetMaterial()->Compile());
        EXPECT_FALSE(testData.GetMaterial()->GetShaderCollection(Name{"TestPipeline"})[Name{"TestShader"}].IsEnabled());
    }

}
