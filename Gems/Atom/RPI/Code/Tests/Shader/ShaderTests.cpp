/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Atom/RHI.Reflect/RenderAttachmentLayoutBuilder.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderAssetCreator.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantTreeAssetCreator.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantAssetCreator.h>

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Shader/Shader.h>

#include <Common/RPITestFixture.h>
#include <Common/ErrorMessageFinder.h>
#include <Common/SerializeTester.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    namespace RPI
    {
        /// This length represents the up-aligned shader variant key length in respect to the shader register space
        /// AZSLc aligns all keys up to a register length and this constant emulates that requirement
        static constexpr uint32_t ShaderVariantKeyAlignedBitCount = (ShaderVariantKeyBitCount % ShaderRegisterBitSize == 0) ?
            ShaderVariantKeyBitCount :
            ShaderVariantKeyBitCount + (ShaderRegisterBitSize - ShaderVariantKeyBitCount % ShaderRegisterBitSize);

        class ShaderAssetTester
            : public UnitTest::SerializeTester<ShaderAsset>
        {
            using Base = UnitTest::SerializeTester<ShaderAsset>;
        public:
            ShaderAssetTester(AZ::SerializeContext* serializeContext)
                : Base(serializeContext)
            {}

            AZ::Data::Asset<ShaderAsset> SerializeInHelper(const AZ::Data::AssetId& assetId)
            {
                AZ::Data::Asset<ShaderAsset> asset = Base::SerializeIn(assetId);
                asset->SelectShaderApiData();
                asset->SetReady();
                return asset;
            }
        };
    }
}

namespace UnitTest
{
    using namespace AZ;

    using ShaderByteCode = AZStd::vector<uint8_t>;

    class TestPipelineLayoutDescriptor
        : public AZ::RHI::PipelineLayoutDescriptor
    {
    public:
        AZ_RTTI(TestPipelineLayoutDescriptor, "{B226636F-7C85-4500-B499-26C112D1128B}", AZ::RHI::PipelineLayoutDescriptor);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestPipelineLayoutDescriptor, AZ::RHI::PipelineLayoutDescriptor>()
                    ->Version(1)
                    ;
            }
        }

        static AZ::RHI::Ptr<TestPipelineLayoutDescriptor> Create()
        {
            return aznew TestPipelineLayoutDescriptor;
        }
    };

    class TestShaderStageFunction
        : public AZ::RHI::ShaderStageFunction
    {
    public:
        AZ_RTTI(TestShaderStageFunction, "{1BAEE536-96CA-4AEB-BA73-D5D72EE35B45}", AZ::RHI::ShaderStageFunction);
        AZ_CLASS_ALLOCATOR(TestShaderStageFunction, AZ::SystemAllocator)

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestShaderStageFunction, AZ::RHI::ShaderStageFunction>()
                    ->Version(1)
                    ->Field("m_byteCode", &TestShaderStageFunction::m_byteCode)
                    ->Field("m_index", &TestShaderStageFunction::m_index)
                    ;
            }
        }

        TestShaderStageFunction() = default;

        explicit TestShaderStageFunction(AZ::RHI::ShaderStage shaderStage)
            : AZ::RHI::ShaderStageFunction(shaderStage)
        {}

        void SetIndex(uint32_t index)
        {
            m_index = index;
        }

        int32_t m_index;

        ShaderByteCode m_byteCode;

    private:
        AZ::RHI::ResultCode FinalizeInternal() override
        {
            SetHash(AZ::TypeHash64(reinterpret_cast<const uint8_t*>(m_byteCode.data()), m_byteCode.size()));
            return AZ::RHI::ResultCode::Success;
        }
    };

    class ShaderTests
        : public RPITestFixture
    {
    protected:
        enum class SpecializationType
        {
            None = 0,
            Partial,
            Full,
            Count
        };

        static const uint32_t SpecializationTypeCount = static_cast<uint32_t>(SpecializationType::Count);

        void SetUp() override
        {
            using namespace AZ;

            RPITestFixture::SetUp();

            auto* serializeContext = GetSerializeContext();
            TestPipelineLayoutDescriptor::Reflect(serializeContext);
            TestShaderStageFunction::Reflect(serializeContext);

            // Example of unscoped enum
            AZStd::vector<RPI::ShaderOptionValuePair> idList0;
            idList0.push_back({ Name("Black"),      RPI::ShaderOptionValue(0) }); // 1+ bit
            idList0.push_back({ Name("Maroon"),     RPI::ShaderOptionValue(1) }); // ...
            idList0.push_back({ Name("Green"),      RPI::ShaderOptionValue(2) }); // 2+ bits
            idList0.push_back({ Name("Olive"),      RPI::ShaderOptionValue(3) }); // ...
            idList0.push_back({ Name("Navy"),       RPI::ShaderOptionValue(4) }); // 3+ bits
            idList0.push_back({ Name("Purple"),     RPI::ShaderOptionValue(5) }); // ...
            idList0.push_back({ Name("Teal"),       RPI::ShaderOptionValue(6) }); // ...
            idList0.push_back({ Name("Silver"),     RPI::ShaderOptionValue(7) }); // ...
            idList0.push_back({ Name("Gray"),       RPI::ShaderOptionValue(8) }); // 4+ bits
            idList0.push_back({ Name("Red"),        RPI::ShaderOptionValue(9) }); // ...
            idList0.push_back({ Name("Lime"),       RPI::ShaderOptionValue(10) }); // ...
            idList0.push_back({ Name("Yellow"),     RPI::ShaderOptionValue(11) }); // ...
            idList0.push_back({ Name("Blue"),       RPI::ShaderOptionValue(12) }); // ...
            idList0.push_back({ Name("Fuchsia"),    RPI::ShaderOptionValue(13) }); // ...
            idList0.push_back({ Name("Cyan"),       RPI::ShaderOptionValue(14) }); // ...
            idList0.push_back({ Name("White"),      RPI::ShaderOptionValue(15) }); // ...

            uint32_t bitOffset = 0;
            uint32_t order = 0;

            m_bindings[0] = RPI::ShaderOptionDescriptor{ Name("Color"),
                                                          RPI::ShaderOptionType::Enumeration,
                                                          bitOffset,
                                                          order++,
                                                          idList0,
                                                          Name("Fuchsia") };
            bitOffset = m_bindings[0].GetBitOffset() + m_bindings[0].GetBitCount();

            // Example of scoped enum - the only difference is that enumerators are qualified
            AZStd::vector<RPI::ShaderOptionValuePair> idList1;
            idList1.push_back({ Name("Quality::Auto"),      RPI::ShaderOptionValue(0) }); // 1+ bit
            idList1.push_back({ Name("Quality::Poor"),      RPI::ShaderOptionValue(1) }); // ...
            idList1.push_back({ Name("Quality::Low"),       RPI::ShaderOptionValue(2) }); // 2+ bits
            idList1.push_back({ Name("Quality::Average"),   RPI::ShaderOptionValue(3) }); // ...
            idList1.push_back({ Name("Quality::Good"),      RPI::ShaderOptionValue(4) }); // 3+ bits
            idList1.push_back({ Name("Quality::High"),      RPI::ShaderOptionValue(5) }); // ...
            idList1.push_back({ Name("Quality::Ultra"),     RPI::ShaderOptionValue(6) }); // ...
            idList1.push_back({ Name("Quality::Sublime"),   RPI::ShaderOptionValue(7) }); // ...

            m_bindings[1] = RPI::ShaderOptionDescriptor{ Name("Quality"),
                                                          RPI::ShaderOptionType::Enumeration,
                                                          bitOffset,
                                                          order++,
                                                          idList1,
                                                          Name("Quality::Auto") };
            bitOffset = m_bindings[1].GetBitOffset() + m_bindings[1].GetBitCount();

            // Example of integer range. It only requires two values, min and max. The name id-s are expected to match the numericla value.
            AZStd::vector<RPI::ShaderOptionValuePair> idList2;
            idList2.push_back({ Name("5"),     RPI::ShaderOptionValue(5) }); // 1+ bit
            idList2.push_back({ Name("200"),   RPI::ShaderOptionValue(200) }); // 8+ bits
            idList2.push_back({ Name("10"),    RPI::ShaderOptionValue(10) }); // It doesn't really matter whether there are extra numbers; the shader option will take the min and max

            m_bindings[2] = RPI::ShaderOptionDescriptor{ Name("NumberSamples"),
                                                          RPI::ShaderOptionType::IntegerRange,
                                                          bitOffset,
                                                          order++,
                                                          idList2,
                                                          Name("50") };
            bitOffset = m_bindings[2].GetBitOffset() + m_bindings[2].GetBitCount();

            // Example of boolean. By standard, the first value should be false (0).
            AZStd::vector<RPI::ShaderOptionValuePair> idList3;
            idList3.push_back({ Name("Off"),  RPI::ShaderOptionValue(0) }); // 1+ bit
            idList3.push_back({ Name("On"),   RPI::ShaderOptionValue(1) }); // ...

            m_bindings[3] = RPI::ShaderOptionDescriptor{ Name("Raytracing"),
                                                          RPI::ShaderOptionType::Boolean,
                                                          bitOffset,
                                                          order++,
                                                          idList3,
                                                          Name("Off") };
            bitOffset = m_bindings[3].GetBitOffset() + m_bindings[3].GetBitCount();

            AZStd::vector<RPI::ShaderOptionValuePair> idList4;
            idList4.push_back({ Name("True"), RPI::ShaderOptionValue(0) }); // 1+ bit
            idList4.push_back({ Name("False"), RPI::ShaderOptionValue(1) }); // ...

            for (uint32_t i = 0; i < m_bindingsFullSpecialization.size(); ++i)
            {
                m_bindingsFullSpecialization[i] = RPI::ShaderOptionDescriptor{
                    Name{ AZStd::to_string(i) }, RPI::ShaderOptionType::Boolean, i, i, idList4, Name("True"), 0,
                                                 aznumeric_caster(i) };
            }

            for (uint32_t i = 0; i < m_bindingsPartialSpecialization.size(); ++i)
            {
                m_bindingsPartialSpecialization[i] = RPI::ShaderOptionDescriptor{ Name{ AZStd::to_string(i) },
                                                                                  RPI::ShaderOptionType::Boolean,
                                                                                  i,
                                                                                  i,
                                                                                  idList4,
                                                                                  Name("True"),
                                                                                  0,
                                                                                  aznumeric_caster(i % 2) ? aznumeric_caster(i) : -1 };
            }

            m_name = Name("TestName");
            m_drawListName = Name("DrawListTagName");
            m_pipelineLayoutDescriptor = TestPipelineLayoutDescriptor::Create();
            m_shaderOptionGroupLayoutForAsset = CreateShaderOptionLayout();
            m_shaderOptionGroupLayoutForAssetPartialSpecialization = CreateShaderOptionLayout({}, SpecializationType::Partial);
            m_shaderOptionGroupLayoutForAssetFullSpecialization = CreateShaderOptionLayout({}, SpecializationType::Full);
            m_shaderOptionGroupLayoutForVariants = m_shaderOptionGroupLayoutForAsset;

            // Just set up a couple values, not the whole struct, for some basic checking later that the struct is copied.
            m_renderStates.m_rasterState.m_fillMode = RHI::FillMode::Wireframe;
            m_renderStates.m_multisampleState.m_samples = 4;
            m_renderStates.m_depthStencilState.m_depth.m_func = RHI::ComparisonFunc::Equal;
            m_renderStates.m_depthStencilState.m_stencil.m_enable = 1;
            m_renderStates.m_blendState.m_targets[0].m_blendOp = RHI::BlendOp::SubtractReverse;


            for (size_t i = 0; i < RHI::Limits::Pipeline::ShaderResourceGroupCountMax; ++i)
            {
                RHI::Ptr<RHI::ShaderResourceGroupLayout> srgLayout = CreateShaderResourceGroupLayout(i);
                AZ::RHI::ShaderResourceGroupBindingInfo bindingInfo = CreateShaderResouceGroupBindingInfo(i);

                m_pipelineLayoutDescriptor->AddShaderResourceGroupLayoutInfo(*srgLayout.get(), bindingInfo);
                m_srgLayouts.push_back(srgLayout);
            }

            m_pipelineLayoutDescriptor->Finalize();
        }

        void TearDown() override
        {
            m_name = Name{};
            m_drawListName = Name{};

            for (size_t i = 0; i < m_bindings.size(); ++i)
            {
                m_bindings[i] = {};
                m_bindingsFullSpecialization[i] = {};
                m_bindingsPartialSpecialization[i] = {};
            }

            m_srgLayouts.clear();
            m_pipelineLayoutDescriptor = nullptr;
            m_shaderOptionGroupLayoutForAsset = nullptr;
            m_shaderOptionGroupLayoutForAssetPartialSpecialization = nullptr;
            m_shaderOptionGroupLayoutForAssetFullSpecialization = nullptr;
            m_shaderOptionGroupLayoutForVariants = nullptr;

            RPITestFixture::TearDown();
        }

        const AZ::RPI::ShaderOptionDescriptor& GetShaderOptionDescriptor(SpecializationType specializationType, uint32_t index)
        {
            switch (specializationType)
            {
            case SpecializationType::Partial:
                return m_bindingsPartialSpecialization[index];
            case SpecializationType::Full:
                return m_bindingsFullSpecialization[index];
            case SpecializationType::None:
            default:
                return m_bindings[index];            
            }
        }

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> CreateShaderOptionLayout(
            AZ::RHI::Handle<size_t> indexToOmit = {}, SpecializationType specializationType = SpecializationType::None)
        {
            using namespace AZ;

            RPI::Ptr<RPI::ShaderOptionGroupLayout> layout = RPI::ShaderOptionGroupLayout::Create();
            for (uint32_t i = 0; i < m_bindings.size(); ++i)
            {
                // Allows omitting a single option to test for missing options.
                if (indexToOmit.GetIndex() != i)
                {
                    layout->AddShaderOption(GetShaderOptionDescriptor(specializationType, i));
                }
            }
            layout->Finalize();

            return layout;
        }

        AZ::Name CreateShaderResourceGroupId(size_t index)
        {
            using namespace AZ;

            return Name{ AZStd::to_string(index) };
        }

        RHI::Ptr<RHI::ShaderResourceGroupLayout> CreateShaderResourceGroupLayout(size_t index)
        {
            using namespace AZ;

            Name srgId = CreateShaderResourceGroupId(index);

            // Creates a simple SRG asset with a unique SRG layout hash (based on the index).

            RHI::Ptr<RHI::ShaderResourceGroupLayout> srgLayout = RHI::ShaderResourceGroupLayout::Create();
            srgLayout->SetName(srgId);
            srgLayout->SetBindingSlot(aznumeric_caster(index));
            srgLayout->AddShaderInput(RHI::ShaderInputBufferDescriptor{
                srgId, RHI::ShaderInputBufferAccess::Read, RHI::ShaderInputBufferType::Raw, 1, 4, static_cast<uint32_t>(index), static_cast<uint32_t>(index)});

            EXPECT_TRUE(srgLayout->Finalize());

            return srgLayout;
        }

        AZ::RHI::ShaderResourceGroupBindingInfo CreateShaderResouceGroupBindingInfo(size_t index)
        {
            Name srgId = CreateShaderResourceGroupId(index);
            AZ::RHI::ShaderResourceGroupBindingInfo bindingInfo;
            bindingInfo.m_resourcesRegisterMap.insert({ srgId, RHI::ResourceBindingInfo{RHI::ShaderStageMask::Vertex, static_cast<uint32_t>(index), static_cast<uint32_t>(index)} });
            return bindingInfo;
        }

        AZ::RPI::ShaderInputContract CreateSimpleShaderInputContract()
        {
            AZ::RPI::ShaderInputContract contract;
            AZ::RPI::ShaderInputContract::StreamChannelInfo channel;
            channel.m_semantic = AZ::RHI::ShaderSemantic{ AZ::Name{"POSITION"} };
            contract.m_streamChannels.push_back(channel);
            return contract;
        }

        AZ::RPI::ShaderOutputContract CreateSimpleShaderOutputContract()
        {
            AZ::RPI::ShaderOutputContract contract;
            AZ::RPI::ShaderOutputContract::ColorAttachmentInfo attachment;
            attachment.m_componentCount = 4;
            contract.m_requiredColorAttachments.push_back(attachment);
            return contract;
        }

        RPI::ShaderVariantListSourceData::VariantInfo CreateVariantInfo(uint32_t stableId, AZStd::vector<AZStd::string> optionValues)
        {
            RPI::ShaderVariantListSourceData::VariantInfo variantInfo;

            variantInfo.m_stableId = stableId;

            auto nextValue = optionValues.begin();
            auto nextOption = m_shaderOptionGroupLayoutForVariants->GetShaderOptions().begin();

            while (nextValue != optionValues.end() &&
                nextOption != m_shaderOptionGroupLayoutForVariants->GetShaderOptions().end())
            {
                if (nextValue->empty())
                {
                    // TODO (To consider) If we decide to support gaps (unqualified options) in the lookup key
                    //  we can actually remove this check
                    variantInfo.m_options[nextOption->GetName()] = nextOption->GetDefaultValue();
                }
                else
                {
                    variantInfo.m_options[nextOption->GetName()] = Name{*nextValue};
                }
                nextValue++;
                nextOption++;
            }

            return variantInfo;
        }

        // Creates and returns a shader option group with the specified option values.
        RPI::ShaderOptionGroup CreateShaderOptionGroup(AZStd::vector<Name> optionValues)
        {
            RPI::ShaderOptionGroup shaderOptionGroup(m_shaderOptionGroupLayoutForVariants);

            auto nextValue = optionValues.begin();
            auto nextOption = m_shaderOptionGroupLayoutForVariants->GetShaderOptions().begin();

            while (nextValue != optionValues.end() &&
                nextOption != m_shaderOptionGroupLayoutForVariants->GetShaderOptions().end())
            {
                if (nextValue->IsEmpty())
                {
                    // TODO (To consider) If we decide to support gaps (unqualified options) in the lookup key
                    //  we can actually remove this check
                    shaderOptionGroup.SetValue(nextOption->GetName(), nextOption->GetDefaultValue());
                }
                else
                {
                    shaderOptionGroup.SetValue(nextOption->GetName(), *nextValue);
                }
                nextValue++;
                nextOption++;
            }

            return shaderOptionGroup;
        }

        Data::Asset<RPI::ShaderVariantAsset> CreateTestShaderVariantAsset(RPI::ShaderVariantId id, RPI::ShaderVariantStableId stableId,
            bool isFullyBaked,
            const AZStd::vector<RHI::ShaderStage>& stagesToActivate = {RHI::ShaderStage::Vertex, RHI::ShaderStage::Fragment})
        {
            RPI::ShaderVariantAssetCreator shaderVariantAssetCreator;
            shaderVariantAssetCreator.Begin(Uuid::CreateRandom(), id, stableId, isFullyBaked);

            for (RHI::ShaderStage rhiStage : stagesToActivate)
            {
                RHI::Ptr<RHI::ShaderStageFunction> vertexStageFunction = aznew TestShaderStageFunction(rhiStage);
                shaderVariantAssetCreator.SetShaderFunction(rhiStage, vertexStageFunction);
            }

            Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset;
            shaderVariantAssetCreator.End(shaderVariantAsset);

            return shaderVariantAsset;
        }

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> GetShaderOptionGroupForAssets(SpecializationType specializationType)
        {
            switch (specializationType)
            {
            case SpecializationType::None:
                return m_shaderOptionGroupLayoutForAsset;
            case SpecializationType::Partial:
                return m_shaderOptionGroupLayoutForAssetPartialSpecialization;
            case SpecializationType::Full:
                return m_shaderOptionGroupLayoutForAssetFullSpecialization;
            default:
                return nullptr;
            }
        }

        void BeginCreatingTestShaderAsset(AZ::RPI::ShaderAssetCreator& creator,
            const AZStd::vector<RHI::ShaderStage>& stagesToActivate = {RHI::ShaderStage::Vertex, RHI::ShaderStage::Fragment},
            SpecializationType specializationType = SpecializationType::None)
        {
            using namespace AZ;

            creator.Begin(Uuid::CreateRandom());
            creator.SetName(m_name);
            creator.SetDrawListName(m_drawListName);
            creator.SetShaderOptionGroupLayout(GetShaderOptionGroupForAssets(specializationType));

            creator.BeginAPI(RHI::Factory::Get().GetType());

            creator.BeginSupervariant(AZ::Name{}); // The default (first) supervariant MUST be nameless.

            creator.SetSrgLayoutList(m_srgLayouts);
            creator.SetPipelineLayout(m_pipelineLayoutDescriptor);

            creator.SetRenderStates(m_renderStates);
            creator.SetInputContract(CreateSimpleShaderInputContract());
            creator.SetOutputContract(CreateSimpleShaderOutputContract());

            creator.SetUseSpecializationConstants(specializationType != SpecializationType::None);

            RHI::ShaderStageAttributeMapList attributeMaps;
            attributeMaps.resize(RHI::ShaderStageCount);
            creator.SetShaderStageAttributeMapList(attributeMaps);

            Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset = CreateTestShaderVariantAsset(RPI::ShaderVariantId{}, RPI::ShaderVariantStableId{0}, false, stagesToActivate);

            creator.SetRootShaderVariantAsset(shaderVariantAsset);

            creator.EndSupervariant();
        }

        //! Used to finish creating a shader that began with BeginCreatingTestShaderAsset(). Call this after adding all the desired shader variants.
        AZ::Data::Asset<AZ::RPI::ShaderAsset> EndCreatingTestShaderAsset(RPI::ShaderAssetCreator& creator)
        {
            Data::Asset<RPI::ShaderAsset> shaderAsset;
            if (creator.EndAPI())
            {
                creator.End(shaderAsset);
            }

            return shaderAsset;
        }

        AZ::Data::Asset<AZ::RPI::ShaderAsset> CreateShaderAsset()
        {
            using namespace AZ;

            RPI::ShaderAssetCreator creator;
            BeginCreatingTestShaderAsset(creator);

            Data::Asset<RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);
            return shaderAsset;
        }

        //! The tree will only contain the root variant.
        AZ::Data::Asset<AZ::RPI::ShaderVariantTreeAsset> CreateEmptyShaderVariantTreeAsset(Data::Asset<RPI::ShaderAsset> shaderAsset)
        {
            using namespace AZ;

            AZStd::vector<RPI::ShaderVariantListSourceData::VariantInfo> shaderVariantList;

            RPI::ShaderVariantTreeAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());
            creator.SetShaderOptionGroupLayout(*shaderAsset->GetShaderOptionGroupLayout());
            creator.SetVariantInfos(shaderVariantList);
            Data::Asset<RPI::ShaderVariantTreeAsset> shaderVariantTreeAsset;
            if (!creator.End(shaderVariantTreeAsset))
            {
                return {};
            }
            return shaderVariantTreeAsset;
        }

        AZ::Data::Asset<AZ::RPI::ShaderVariantTreeAsset> CreateShaderVariantTreeAssetForSearch(Data::Asset<RPI::ShaderAsset> shaderAsset)
        {
            using namespace AZ;

            AZStd::vector<RPI::ShaderVariantListSourceData::VariantInfo> shaderVariantList;
            shaderVariantList.push_back(CreateVariantInfo(1, { AZStd::string{"Fuchsia"} }));
            shaderVariantList.push_back(CreateVariantInfo(2, { AZStd::string{"Fuchsia"}, AZStd::string{"Quality::Auto"} }));
            shaderVariantList.push_back(CreateVariantInfo(3, { AZStd::string{"Fuchsia"}, AZStd::string{"Quality::Auto"}, AZStd::string{"50"} }));
            shaderVariantList.push_back(CreateVariantInfo(4, { AZStd::string{"Fuchsia"}, AZStd::string{"Quality::Auto"}, AZStd::string{"50"}, AZStd::string{"Off"} }));
            shaderVariantList.push_back(CreateVariantInfo(5, { AZStd::string{"Fuchsia"}, AZStd::string{"Quality::Auto"}, AZStd::string{"50"}, AZStd::string{"On"} }));
            shaderVariantList.push_back(CreateVariantInfo(6, { AZStd::string{"Teal"} }));
            shaderVariantList.push_back(CreateVariantInfo(7, { AZStd::string{"Teal"}, AZStd::string{"Quality::Sublime"} }));

            RPI::ShaderVariantTreeAssetCreator creator;
            creator.Begin(Uuid::CreateRandom()) ;
            creator.SetShaderOptionGroupLayout(*shaderAsset->GetShaderOptionGroupLayout());
            creator.SetVariantInfos(shaderVariantList);
            Data::Asset<RPI::ShaderVariantTreeAsset> shaderVariantTreeAsset;
            if (!creator.End(shaderVariantTreeAsset))
            {
                return {};
            }
            return shaderVariantTreeAsset;
        }

        void ValidateShaderAsset(const AZ::Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset)
        {
            using namespace AZ;

            EXPECT_TRUE(shaderAsset);

            EXPECT_EQ(shaderAsset->GetName(), m_name);
            EXPECT_EQ(shaderAsset->GetDrawListName(), m_drawListName);
            EXPECT_EQ(shaderAsset->GetShaderOptionGroupLayout()->GetHash(), m_shaderOptionGroupLayoutForAsset->GetHash());
            EXPECT_EQ(shaderAsset->GetPipelineLayoutDescriptor()->GetHash(), m_pipelineLayoutDescriptor->GetHash());

            for (size_t i = 0; i < shaderAsset->GetShaderResourceGroupLayouts().size(); ++i)
            {
                auto srgLayouts = shaderAsset->GetShaderResourceGroupLayouts();
                auto& srgLayout = srgLayouts[i];
                EXPECT_EQ(srgLayout->GetHash(), m_srgLayouts[i]->GetHash());
                EXPECT_EQ(shaderAsset->FindShaderResourceGroupLayout(CreateShaderResourceGroupId(i))->GetHash(), srgLayout->GetHash());
            }
        }

        void ValidateShader(const AZ::Data::Instance<AZ::RPI::Shader>& shader)
        {
            using namespace AZ;

            EXPECT_TRUE(shader);
            EXPECT_TRUE(shader->GetAsset());

            auto shaderAsset = shader->GetAsset();
            EXPECT_EQ(shader->GetPipelineStateType(), shaderAsset->GetPipelineStateType());
            using ShaderResourceGroupLayoutSpan = AZStd::span<const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>>;
            ShaderResourceGroupLayoutSpan shaderResourceGroupLayoutSpan = shader->GetShaderResourceGroupLayouts();
            ShaderResourceGroupLayoutSpan shaderAssetResourceGroupLayoutSpan = shader->GetShaderResourceGroupLayouts();
            EXPECT_EQ(shaderResourceGroupLayoutSpan.data(), shaderAssetResourceGroupLayoutSpan.data());
            EXPECT_EQ(shaderResourceGroupLayoutSpan.size(), shaderAssetResourceGroupLayoutSpan.size());

            const RPI::ShaderVariant& rootShaderVariant = shader->GetVariant( RPI::ShaderVariantStableId{0} );

            RHI::PipelineStateDescriptorForDraw descriptorForDraw;
            rootShaderVariant.ConfigurePipelineState(descriptorForDraw);

            EXPECT_EQ(descriptorForDraw.m_pipelineLayoutDescriptor->GetHash(), m_pipelineLayoutDescriptor->GetHash());
            EXPECT_NE(descriptorForDraw.m_vertexFunction, nullptr);
            EXPECT_NE(descriptorForDraw.m_fragmentFunction, nullptr);
            EXPECT_EQ(descriptorForDraw.m_renderStates.GetHash(), m_renderStates.GetHash());
            EXPECT_EQ(descriptorForDraw.m_inputStreamLayout.GetHash(), HashValue64{ 0 }); // ConfigurePipelineState shouldn't touch descriptorForDraw.m_inputStreamLayout
            EXPECT_EQ(descriptorForDraw.m_renderAttachmentConfiguration.GetHash(), RHI::RenderAttachmentConfiguration().GetHash()); // ConfigurePipelineState shouldn't touch descriptorForDraw.m_outputAttachmentLayout

            // Actual layout content doesn't matter for this test, it just needs to be set up to pass validation inside AcquirePipelineState().
            descriptorForDraw.m_inputStreamLayout.SetTopology(RHI::PrimitiveTopology::TriangleList);
            descriptorForDraw.m_inputStreamLayout.Finalize();
            RHI::RenderAttachmentLayoutBuilder builder;
            builder.AddSubpass()
                ->RenderTargetAttachment(RHI::Format::R8G8B8A8_SNORM)
                ->DepthStencilAttachment(RHI::Format::R32_FLOAT);
            builder.End(descriptorForDraw.m_renderAttachmentConfiguration.m_renderAttachmentLayout);
            
            const RHI::PipelineState* pipelineState = shader->AcquirePipelineState(descriptorForDraw);
            EXPECT_NE(pipelineState, nullptr);
        }

        AZStd::array<AZ::RPI::ShaderOptionDescriptor, 4> m_bindings;
        AZStd::array<AZ::RPI::ShaderOptionDescriptor, 4> m_bindingsFullSpecialization;
        AZStd::array<AZ::RPI::ShaderOptionDescriptor, 4> m_bindingsPartialSpecialization;


        AZ::Name m_name;
        AZ::Name m_drawListName;
        AZ::RHI::Ptr<AZ::RHI::PipelineLayoutDescriptor> m_pipelineLayoutDescriptor;
        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> m_shaderOptionGroupLayoutForAsset;
        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> m_shaderOptionGroupLayoutForAssetPartialSpecialization;
        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> m_shaderOptionGroupLayoutForAssetFullSpecialization;
        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> m_shaderOptionGroupLayoutForVariants;

        AZ::RHI::RenderStates m_renderStates;

        AZStd::fixed_vector<AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>, AZ::RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgLayouts;
    };

    TEST_F(ShaderTests, ShaderOptionBindingTest)
    {
        using namespace AZ;

        EXPECT_EQ(m_bindings[0].GetBitMask(), RPI::ShaderVariantKey{ AZ_BIT_MASK_OFFSET(4, 0) });
        EXPECT_EQ(m_bindings[1].GetBitMask(), RPI::ShaderVariantKey{ AZ_BIT_MASK_OFFSET(3, 4) });
        EXPECT_EQ(m_bindings[2].GetBitMask(), RPI::ShaderVariantKey{ AZ_BIT_MASK_OFFSET(8, 7) });
        EXPECT_EQ(m_bindings[3].GetBitMask(), RPI::ShaderVariantKey{ AZ_BIT_MASK_OFFSET(1, 15) });


        EXPECT_TRUE(m_bindings[0].FindValue(Name("Navy")).IsValid());
        EXPECT_FALSE(m_bindings[0].FindValue(Name("Color::Navy")).IsValid());    // Not found - Color is unscoped

        EXPECT_TRUE(m_bindings[1].FindValue(Name("Quality::Average")).IsValid());
        EXPECT_FALSE(m_bindings[1].FindValue(Name("Average")).IsValid());        // Not found - Quality is scoped
        EXPECT_FALSE(m_bindings[1].FindValue(Name("Cake")).IsValid());           // Not found - Cake is not on the list
        EXPECT_FALSE(m_bindings[1].FindValue(Name("Quality::Cake")).IsValid());  // Not found - still not on the list

        EXPECT_TRUE(m_bindings[2].FindValue(Name("5")).IsValid());
        EXPECT_TRUE(m_bindings[2].FindValue(Name("200")).IsValid());
        EXPECT_TRUE(m_bindings[2].FindValue(Name("42")).IsValid());
        EXPECT_FALSE(m_bindings[2].FindValue(Name("-1")).IsValid());              // Not found - less than MinValue
        EXPECT_FALSE(m_bindings[2].FindValue(Name("1001")).IsValid());           // Not found - more than MaxValue

        EXPECT_TRUE(m_bindings[3].FindValue(Name("Off")).IsValid());
        EXPECT_TRUE(m_bindings[3].FindValue(Name("On")).IsValid());
        EXPECT_FALSE(m_bindings[3].FindValue(Name("False")).IsValid());          // Not found - the correct user-defined id is Off
        EXPECT_FALSE(m_bindings[3].FindValue(Name("True")).IsValid());           // Not found - the correct user-defined id is On

        EXPECT_EQ(m_bindings[0].GetValueName(RPI::ShaderOptionValue(4)), Name("Navy"));
        EXPECT_EQ(m_bindings[1].GetValueName(RPI::ShaderOptionValue(3)), Name("Quality::Average"));
        EXPECT_EQ(m_bindings[2].GetValueName(RPI::ShaderOptionValue(200)), Name("200"));
        EXPECT_EQ(m_bindings[3].GetValueName(RPI::ShaderOptionValue(0)), Name("Off"));
        EXPECT_EQ(m_bindings[3].GetValueName(RPI::ShaderOptionValue(1)), Name("On"));
        EXPECT_TRUE(m_bindings[2].GetValueName(RPI::ShaderOptionValue(-1)).IsEmpty());           // No matching value
        EXPECT_TRUE(m_bindings[2].GetValueName(RPI::ShaderOptionValue(1001)).IsEmpty());           // No matching value

        RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();

        bool success = shaderOptionGroupLayout->AddShaderOption(m_bindings[0]);
        EXPECT_TRUE(success);

        success = shaderOptionGroupLayout->AddShaderOption(m_bindings[1]);
        EXPECT_TRUE(success);

        success = shaderOptionGroupLayout->AddShaderOption(m_bindings[2]);
        EXPECT_TRUE(success);

        success = shaderOptionGroupLayout->AddShaderOption(m_bindings[3]);
        EXPECT_TRUE(success);

        shaderOptionGroupLayout->Finalize();
        EXPECT_TRUE(shaderOptionGroupLayout->IsFinalized());

        RPI::ShaderOptionGroup testGroup(shaderOptionGroupLayout);

        m_bindings[0].Set(testGroup, m_bindings[0].FindValue(Name("Gray")));
        EXPECT_EQ(m_bindings[0].Get(testGroup).GetIndex(), RPI::ShaderOptionValue(8).GetIndex());

        m_bindings[0].Set(testGroup, RPI::ShaderOptionValue(1));
        EXPECT_EQ(m_bindings[0].Get(testGroup).GetIndex(), RPI::ShaderOptionValue(1).GetIndex());

        testGroup.SetValue(Name("Color"), Name("Olive"));
        EXPECT_EQ(testGroup.GetValue(Name("Color")).GetIndex(), RPI::ShaderOptionValue(3).GetIndex());

        testGroup.SetValue(Name("Color"), RPI::ShaderOptionValue(5));
        EXPECT_EQ(testGroup.GetValue(Name("Color")).GetIndex(), RPI::ShaderOptionValue(5).GetIndex());

        testGroup.SetValue(RPI::ShaderOptionIndex(0), Name("Lime"));
        EXPECT_EQ(testGroup.GetValue(RPI::ShaderOptionIndex(0)).GetIndex(), RPI::ShaderOptionValue(10).GetIndex());

        testGroup.SetValue(RPI::ShaderOptionIndex(0), RPI::ShaderOptionValue(0));
        EXPECT_EQ(testGroup.GetValue(RPI::ShaderOptionIndex(0)).GetIndex(), RPI::ShaderOptionValue(0).GetIndex());

        m_bindings[1].Set(testGroup, m_bindings[1].FindValue(Name("Quality::Average")));
        EXPECT_EQ(m_bindings[1].Get(testGroup).GetIndex(), RPI::ShaderOptionValue(3).GetIndex());

        m_bindings[1].Set(testGroup, RPI::ShaderOptionValue(1));
        EXPECT_EQ(m_bindings[1].Get(testGroup).GetIndex(), RPI::ShaderOptionValue(1).GetIndex());

        testGroup.SetValue(Name("Quality"), Name("Quality::Ultra"));
        EXPECT_EQ(testGroup.GetValue(Name("Quality")).GetIndex(), RPI::ShaderOptionValue(6).GetIndex());

        testGroup.SetValue(Name("Quality"), RPI::ShaderOptionValue(5));
        EXPECT_EQ(testGroup.GetValue(Name("Quality")).GetIndex(), RPI::ShaderOptionValue(5).GetIndex());

        testGroup.SetValue(RPI::ShaderOptionIndex(1), Name("Quality::Auto"));
        EXPECT_EQ(testGroup.GetValue(RPI::ShaderOptionIndex(1)).GetIndex(), RPI::ShaderOptionValue(0).GetIndex());

        testGroup.SetValue(RPI::ShaderOptionIndex(1), RPI::ShaderOptionValue(2));
        EXPECT_EQ(testGroup.GetValue(RPI::ShaderOptionIndex(1)).GetIndex(), RPI::ShaderOptionValue(2).GetIndex());


        m_bindings[2].Set(testGroup, m_bindings[2].FindValue(Name("150")));
        EXPECT_EQ(m_bindings[2].Get(testGroup).GetIndex(), RPI::ShaderOptionValue(150).GetIndex());

        m_bindings[2].Set(testGroup, RPI::ShaderOptionValue(120));
        EXPECT_EQ(m_bindings[2].Get(testGroup).GetIndex(), RPI::ShaderOptionValue(120).GetIndex());

        testGroup.SetValue(Name("NumberSamples"), Name("101"));
        EXPECT_EQ(testGroup.GetValue(Name("NumberSamples")).GetIndex(), RPI::ShaderOptionValue(101).GetIndex());

        testGroup.SetValue(Name("NumberSamples"), RPI::ShaderOptionValue(102));
        EXPECT_EQ(testGroup.GetValue(Name("NumberSamples")).GetIndex(), RPI::ShaderOptionValue(102).GetIndex());

        testGroup.SetValue(RPI::ShaderOptionIndex(2), Name("103"));
        EXPECT_EQ(testGroup.GetValue(RPI::ShaderOptionIndex(2)).GetIndex(), RPI::ShaderOptionValue(103).GetIndex());

        testGroup.SetValue(RPI::ShaderOptionIndex(2), RPI::ShaderOptionValue(104));
        EXPECT_EQ(testGroup.GetValue(RPI::ShaderOptionIndex(2)).GetIndex(), RPI::ShaderOptionValue(104).GetIndex());

        // Tests for invalid or Null value id

        // Setting a valid value id changes the key
        testGroup.SetValue(Name("Quality"), Name("Quality::Sublime"));
        EXPECT_EQ(testGroup.GetValue(Name("Quality")).GetIndex(), RPI::ShaderOptionValue(7).GetIndex());

        // "Cake" is delicious, but it's not a valid option for "Quality"
        // Setting an invalid value id does nothing - it's ignored, so the key remains the same
        AZ_TEST_START_TRACE_SUPPRESSION;
        testGroup.SetValue(Name("Quality"), Name("Cake"));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(testGroup.GetValue(Name("Quality")).GetIndex(), RPI::ShaderOptionValue(7).GetIndex());

        // ClearValue clears the mask
        testGroup.ClearValue(Name("Quality"));
        EXPECT_EQ(testGroup.GetValue(Name("Quality")).IsNull(), true);
    }

    TEST_F(ShaderTests, ShaderOptionGroupLayoutTest)
    {
        using namespace AZ;

        RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();

        bool success = shaderOptionGroupLayout->AddShaderOption(m_bindings[0]);
        EXPECT_TRUE(success);

        success = shaderOptionGroupLayout->AddShaderOption(m_bindings[1]);
        EXPECT_TRUE(success);

        success = shaderOptionGroupLayout->AddShaderOption(m_bindings[2]);
        EXPECT_TRUE(success);

        success = shaderOptionGroupLayout->AddShaderOption(m_bindings[3]);
        EXPECT_TRUE(success);

        auto intRangeType = RPI::ShaderOptionType::IntegerRange;

        uint32_t order = m_bindings[3].GetOrder() + 1; // The tests below will fail anyway, but still

        ErrorMessageFinder errorMessageFinder;

        // Overlaps previous mask.
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("mask overlaps with previously added masks");
        AZStd::vector<RPI::ShaderOptionValuePair> list0;
        list0.push_back({ Name("0"),    RPI::ShaderOptionValue(0) }); // 1+ bit
        list0.push_back({ Name("1"),    RPI::ShaderOptionValue(1) }); // ...
        success = shaderOptionGroupLayout->AddShaderOption(AZ::RPI::ShaderOptionDescriptor{ Name{"Invalid"}, intRangeType, 6, order++, list0, Name("0") });
        EXPECT_FALSE(success);
        errorMessageFinder.CheckExpectedErrorsFound();

        // Add shader option that extends past end of bit mask.
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("exceeds size of mask");
        AZStd::vector<RPI::ShaderOptionValuePair> list1;
        list1.push_back({ Name("0"),    RPI::ShaderOptionValue(0) }); // 1+ bit
        list1.push_back({ Name("255"),  RPI::ShaderOptionValue(255) }); // 8+ bit
        success = shaderOptionGroupLayout->AddShaderOption(AZ::RPI::ShaderOptionDescriptor{ Name{"Invalid"}, intRangeType, RPI::ShaderVariantKeyBitCount - 4, order++, list1, Name("0") });
        EXPECT_FALSE(success);
        errorMessageFinder.CheckExpectedErrorsFound();

        // Add shader option with empty name.
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("empty name");
        AZStd::vector<RPI::ShaderOptionValuePair> list2;
        list2.push_back({ Name("0"),    RPI::ShaderOptionValue(0) }); // 1+ bit
        list2.push_back({ Name("1"),    RPI::ShaderOptionValue(1) }); // ...
        success = shaderOptionGroupLayout->AddShaderOption(AZ::RPI::ShaderOptionDescriptor{ Name{}, intRangeType, 16, order++, list2, Name("0") });
        EXPECT_FALSE(success);
        errorMessageFinder.CheckExpectedErrorsFound();

        // Add shader option with empty bits.
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("has zero bits");
        AZStd::vector<RPI::ShaderOptionValuePair> list3;
        success = shaderOptionGroupLayout->AddShaderOption(AZ::RPI::ShaderOptionDescriptor{ Name{"Invalid"}, intRangeType, 16, order++, list3, Name("0") });
        EXPECT_FALSE(success);
        errorMessageFinder.CheckExpectedErrorsFound();

        // An integer range option must have at least two values defining the range
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("has zero bits");
        AZStd::vector<RPI::ShaderOptionValuePair> list3b;
        list3b.push_back({ Name("0"), RPI::ShaderOptionValue(0) }); // 1+ bit
        success = shaderOptionGroupLayout->AddShaderOption(AZ::RPI::ShaderOptionDescriptor{ Name{"Invalid"}, intRangeType, 16, order++, list3b, Name("0") });
        EXPECT_FALSE(success);
        errorMessageFinder.CheckExpectedErrorsFound();

        // Add a shader option with an order that collides with an existing shader option
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("has the same order");
        uint32_t bitOffset = m_bindings[3].GetBitOffset() + m_bindings[3].GetBitCount();
        AZStd::vector<RPI::ShaderOptionValuePair> list4;
        list4.push_back({ Name("0"),    RPI::ShaderOptionValue(0) }); // 1+ bit
        list4.push_back({ Name("1"),    RPI::ShaderOptionValue(1) }); // ...
        success = shaderOptionGroupLayout->AddShaderOption(AZ::RPI::ShaderOptionDescriptor{ Name{"Invalid"}, intRangeType, bitOffset, 0, list4, Name("0") });
        EXPECT_FALSE(success);
        errorMessageFinder.CheckExpectedErrorsFound();

        // Add shader option with an invalid default int value.
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("invalid default value");
        AZStd::vector<RPI::ShaderOptionValuePair> list6;
        list6.push_back({ Name("0"),    RPI::ShaderOptionValue(0) }); // 1+ bit
        list6.push_back({ Name("1"),    RPI::ShaderOptionValue(1) }); // ...
        success = shaderOptionGroupLayout->AddShaderOption(AZ::RPI::ShaderOptionDescriptor{ Name{"Invalid"}, intRangeType, 16, order++, list6, Name("3") });
        EXPECT_FALSE(success);
        errorMessageFinder.CheckExpectedErrorsFound();

        // Add shader option with an invalid default enum value.
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("invalid default value");
        AZStd::vector<RPI::ShaderOptionValuePair> list7;
        list7.push_back({ Name("TypeA"),    RPI::ShaderOptionValue(0) });
        list7.push_back({ Name("TypeB"),   RPI::ShaderOptionValue(1) });
        list7.push_back({ Name("TypeC"),   RPI::ShaderOptionValue(2) });
        success = shaderOptionGroupLayout->AddShaderOption(AZ::RPI::ShaderOptionDescriptor{ Name{"Invalid"}, RPI::ShaderOptionType::Enumeration, 16, order++, list7, Name("TypeO") });
        EXPECT_FALSE(success);
        errorMessageFinder.CheckExpectedErrorsFound();

        // Test access before finalize.
        EXPECT_FALSE(shaderOptionGroupLayout->IsFinalized());
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("ShaderOptionGroupLayout is not finalized", 4);
        EXPECT_EQ(shaderOptionGroupLayout->FindShaderOptionIndex(m_bindings[0].GetName()), RPI::ShaderOptionIndex());
        EXPECT_EQ(shaderOptionGroupLayout->FindShaderOptionIndex(m_bindings[1].GetName()), RPI::ShaderOptionIndex());
        EXPECT_EQ(shaderOptionGroupLayout->FindShaderOptionIndex(m_bindings[2].GetName()), RPI::ShaderOptionIndex());
        EXPECT_EQ(shaderOptionGroupLayout->FindShaderOptionIndex(m_bindings[3].GetName()), RPI::ShaderOptionIndex());
        errorMessageFinder.CheckExpectedErrorsFound();

        {
            errorMessageFinder.Reset();
            errorMessageFinder.AddExpectedErrorMessage("ShaderOptionGroupLayout is not finalized");
            RPI::ShaderVariantKey testKey = 1;
            EXPECT_FALSE(shaderOptionGroupLayout->IsValidShaderVariantKey(testKey));
            errorMessageFinder.CheckExpectedErrorsFound();
        }

        errorMessageFinder.Disable();

        shaderOptionGroupLayout->Finalize();
        EXPECT_TRUE(shaderOptionGroupLayout->IsFinalized());

        EXPECT_EQ(shaderOptionGroupLayout->GetShaderOptionCount(), 4);
        EXPECT_EQ(shaderOptionGroupLayout->GetShaderOption(RPI::ShaderOptionIndex(0)), m_bindings[0]);
        EXPECT_EQ(shaderOptionGroupLayout->GetShaderOption(RPI::ShaderOptionIndex(1)), m_bindings[1]);
        EXPECT_EQ(shaderOptionGroupLayout->GetShaderOption(RPI::ShaderOptionIndex(2)), m_bindings[2]);
        EXPECT_EQ(shaderOptionGroupLayout->GetShaderOption(RPI::ShaderOptionIndex(3)), m_bindings[3]);

        RPI::ShaderVariantKey unionMask;
        for (const auto& binding : m_bindings)
        {
            unionMask |= binding.GetBitMask();
        }
        EXPECT_EQ(unionMask, shaderOptionGroupLayout->GetBitMask());

        EXPECT_TRUE(shaderOptionGroupLayout->IsValidShaderVariantKey(m_bindings[0].GetBitMask()));
        EXPECT_TRUE(shaderOptionGroupLayout->IsValidShaderVariantKey(m_bindings[1].GetBitMask()));
        EXPECT_TRUE(shaderOptionGroupLayout->IsValidShaderVariantKey(m_bindings[2].GetBitMask()));
        EXPECT_TRUE(shaderOptionGroupLayout->IsValidShaderVariantKey(m_bindings[3].GetBitMask()));

        // Test value-lookup functions

        auto& colorOption = shaderOptionGroupLayout->GetShaderOption(RPI::ShaderOptionIndex{ 0 });

        EXPECT_EQ(colorOption.FindValue(Name{ "Navy" }).GetIndex(), 4);
        EXPECT_EQ(colorOption.FindValue(Name{ "Purple" }).GetIndex(), 5);
        EXPECT_FALSE(colorOption.FindValue(Name{ "Blah" }).IsValid());

        EXPECT_EQ(shaderOptionGroupLayout->FindValue(RPI::ShaderOptionIndex{ 0 }, Name{ "Navy" }).GetIndex(), 4);
        EXPECT_EQ(shaderOptionGroupLayout->FindValue(RPI::ShaderOptionIndex{ 0 }, Name{ "Purple" }).GetIndex(), 5);
        EXPECT_EQ(shaderOptionGroupLayout->FindValue(Name{ "Color" }, Name{ "Navy" }).GetIndex(), 4);
        EXPECT_EQ(shaderOptionGroupLayout->FindValue(Name{ "Color" }, Name{ "Purple" }).GetIndex(), 5);

        EXPECT_FALSE(shaderOptionGroupLayout->FindValue(RPI::ShaderOptionIndex{ 0 }, Name{ "Blah" }).IsValid());
        EXPECT_FALSE(shaderOptionGroupLayout->FindValue(Name{ "Color" }, Name{ "Blah" }).IsValid());
        EXPECT_FALSE(shaderOptionGroupLayout->FindValue(RPI::ShaderOptionIndex{}, Name{ "Navy" }).IsValid());
        EXPECT_FALSE(shaderOptionGroupLayout->FindValue(RPI::ShaderOptionIndex{ 100 }, Name{ "Navy" }).IsValid());
        EXPECT_FALSE(shaderOptionGroupLayout->FindValue(Name{ "Blah" }, Name{ "Navy" }).IsValid());

        EXPECT_FALSE(shaderOptionGroupLayout->FindShaderOptionIndex(Name{ "Invalid" }).IsValid());
    }

    TEST_F(ShaderTests, ShaderOptionGroupLayoutSpecializationTest)
    {
        using namespace AZ;
        AZStd::vector<RPI::ShaderOptionValuePair> idList4;
        idList4.push_back({ Name("True"), RPI::ShaderOptionValue(0) });
        idList4.push_back({ Name("False"), RPI::ShaderOptionValue(1) });

        {
            RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();
            bool success = shaderOptionGroupLayout->AddShaderOption(
                RPI::ShaderOptionDescriptor{ Name{ "Specialized1" }, RPI::ShaderOptionType::Boolean, 0, 0, idList4, Name("False"), 0, 0 });
            EXPECT_TRUE(success);
            success = shaderOptionGroupLayout->AddShaderOption(
                RPI::ShaderOptionDescriptor{ Name{ "Specialized2" }, RPI::ShaderOptionType::Boolean, 1, 1, idList4, Name("False"), 0, 1 });
            EXPECT_TRUE(success);
            success = shaderOptionGroupLayout->AddShaderOption(
                RPI::ShaderOptionDescriptor{ Name{ "Specialized3" }, RPI::ShaderOptionType::Boolean, 2, 2, idList4, Name("False"), 0, 2 });
            EXPECT_TRUE(success);
            shaderOptionGroupLayout->Finalize();
            EXPECT_TRUE(shaderOptionGroupLayout->IsFullySpecialized());
            EXPECT_TRUE(shaderOptionGroupLayout->UseSpecializationConstants());
        }

        {
            RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();
            bool success = shaderOptionGroupLayout->AddShaderOption(
                RPI::ShaderOptionDescriptor{ Name{ "Specialized1" }, RPI::ShaderOptionType::Boolean, 0, 0, idList4, Name("False"), 0, 0 });
            EXPECT_TRUE(success);
            success = shaderOptionGroupLayout->AddShaderOption(
                RPI::ShaderOptionDescriptor{ Name{ "Specialized2" }, RPI::ShaderOptionType::Boolean, 1, 1, idList4, Name("False"), 0, -1 });
            EXPECT_TRUE(success);
            success = shaderOptionGroupLayout->AddShaderOption(
                RPI::ShaderOptionDescriptor{ Name{ "Specialized3" }, RPI::ShaderOptionType::Boolean, 2, 2, idList4, Name("False"), 0, 1 });
            EXPECT_TRUE(success);
            shaderOptionGroupLayout->Finalize();
            EXPECT_FALSE(shaderOptionGroupLayout->IsFullySpecialized());
            EXPECT_TRUE(shaderOptionGroupLayout->UseSpecializationConstants());
        }

        {
            RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();
            bool success = shaderOptionGroupLayout->AddShaderOption(
                RPI::ShaderOptionDescriptor{ Name{ "Specialized1" }, RPI::ShaderOptionType::Boolean, 0, 0, idList4, Name("False"), 0, -1 });
            EXPECT_TRUE(success);
            success = shaderOptionGroupLayout->AddShaderOption(
                RPI::ShaderOptionDescriptor{ Name{ "Specialized2" }, RPI::ShaderOptionType::Boolean, 1, 1, idList4, Name("False"), 0, -1 });
            EXPECT_TRUE(success);
            success = shaderOptionGroupLayout->AddShaderOption(
                RPI::ShaderOptionDescriptor{ Name{ "Specialized3" }, RPI::ShaderOptionType::Boolean, 2, 2, idList4, Name("False"), 0, -1 });
            EXPECT_TRUE(success);
            shaderOptionGroupLayout->Finalize();
            EXPECT_FALSE(shaderOptionGroupLayout->IsFullySpecialized());
            EXPECT_FALSE(shaderOptionGroupLayout->UseSpecializationConstants());
        }
    }

    TEST_F(ShaderTests, ImplicitDefaultValue)
    {
        // Add shader option with no default value.

        RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();

        AZStd::vector<RPI::ShaderOptionValuePair> values = AZ::RPI::CreateEnumShaderOptionValues({"A", "B", "C"});
        bool success = shaderOptionGroupLayout->AddShaderOption(AZ::RPI::ShaderOptionDescriptor{ Name{"NoDefaultSpecified"}, RPI::ShaderOptionType::Enumeration, 0, 0, values });
        EXPECT_TRUE(success);
        EXPECT_STREQ("A", shaderOptionGroupLayout->GetShaderOptions().back().GetDefaultValue().GetCStr());
    }

    TEST_F(ShaderTests, ShaderOptionGroupTest)
    {
        using namespace AZ;

        RPI::ShaderOptionGroup group(m_shaderOptionGroupLayoutForAsset);

        EXPECT_TRUE(group.GetShaderVariantId().IsEmpty());

        group.SetValue(RPI::ShaderOptionIndex(0), RPI::ShaderOptionValue(7));
        group.SetValue(RPI::ShaderOptionIndex(1), RPI::ShaderOptionValue(6));
        group.SetValue(RPI::ShaderOptionIndex(2), RPI::ShaderOptionValue(5));
        group.SetValue(RPI::ShaderOptionIndex(3), RPI::ShaderOptionValue(1));

        group.SetValue(group.FindShaderOptionIndex(m_bindings[0].GetName()), RPI::ShaderOptionValue(7));
        group.SetValue(group.FindShaderOptionIndex(m_bindings[1].GetName()), RPI::ShaderOptionValue(6));
        group.SetValue(group.FindShaderOptionIndex(m_bindings[2].GetName()), RPI::ShaderOptionValue(5));
        group.SetValue(group.FindShaderOptionIndex(m_bindings[3].GetName()), RPI::ShaderOptionValue(1));

        EXPECT_FALSE(group.GetShaderVariantId().IsEmpty());
        EXPECT_EQ(group.GetValue(group.FindShaderOptionIndex(m_bindings[0].GetName())).GetIndex(), 7);
        EXPECT_EQ(group.GetValue(group.FindShaderOptionIndex(m_bindings[1].GetName())).GetIndex(), 6);
        EXPECT_EQ(group.GetValue(group.FindShaderOptionIndex(m_bindings[2].GetName())).GetIndex(), 5);
        EXPECT_EQ(group.GetValue(group.FindShaderOptionIndex(m_bindings[3].GetName())).GetIndex(), 1);
        EXPECT_EQ(group.FindShaderOptionIndex(Name{}), RPI::ShaderOptionIndex{});
        EXPECT_EQ(group.FindShaderOptionIndex(Name{ "Invalid" }), RPI::ShaderOptionIndex{});

        // Helper methods - these are suboptimal since because they fetch index from id
        // The intended use for these methods is in prototypes and simple sample code
        group.SetValue(Name("Color"), Name("Fuchsia"));            //   13
        group.SetValue(Name("Quality"), Name("Quality::Sublime"));   //   7
        group.SetValue(Name("NumberSamples"), Name("190"));                // 190
        group.SetValue(Name("Raytracing"), Name("On"));                 //   1

        EXPECT_EQ(group.GetValue(Name("Color")).GetIndex(), 13);
        EXPECT_EQ(group.GetValue(Name("Quality")).GetIndex(), 7);
        EXPECT_EQ(group.GetValue(Name("NumberSamples")).GetIndex(), 190);
        EXPECT_EQ(group.GetValue(Name("Raytracing")).GetIndex(), 1);
    }

    RPI::Ptr<RPI::ShaderOptionGroupLayout> CreateOptionsLayoutWithAllBools()
    {
        AZStd::vector<RPI::ShaderOptionValuePair> boolIdList;
        boolIdList.push_back({Name("Off"),  RPI::ShaderOptionValue(0)});
        boolIdList.push_back({Name("On"),   RPI::ShaderOptionValue(1)});

        RPI::Ptr<RPI::ShaderOptionGroupLayout> layout = RPI::ShaderOptionGroupLayout::Create();
        for (uint32_t i = 0; i < AZ::RPI::ShaderVariantKeyBitCount; ++i)
        {
            RPI::ShaderOptionDescriptor option{
                Name{AZStd::string::format("option%d", i)},
                RPI::ShaderOptionType::Boolean,
                i,
                i,
                boolIdList,
                Name{"Off"}};

            EXPECT_TRUE(layout->AddShaderOption(option));
        }
        layout->Finalize();

        return layout;
    }

    TEST_F(ShaderTests, ShaderOptionGroup_AccessEachBit_AllOtherOptionsUnspecified)
    {
        AZStd::bitset<AZ::RPI::ShaderVariantKeyBitCount> allBitsOff;
        for (size_t i = 0; i < AZ::RPI::ShaderVariantKeyBitCount; ++i)
        {
            // Verify the assumption that bitset is initialized to all false
            EXPECT_FALSE(allBitsOff[i]);
        }

        for (size_t targetBit = 0; targetBit < AZ::RPI::ShaderVariantKeyBitCount; ++targetBit)
        {
            RPI::ShaderOptionGroup group(CreateOptionsLayoutWithAllBools());

            // Set target bit on, all other bits are unspecified
            group.SetValue(AZ::RPI::ShaderOptionIndex{targetBit}, AZ::RPI::ShaderOptionValue{1});

            for (int j = 0; j < AZ::RPI::ShaderVariantKeyBitCount; ++j)
            {
                if (j == targetBit)
                {
                    EXPECT_TRUE(group.GetValue(AZ::RPI::ShaderOptionIndex{j}).IsValid());
                    EXPECT_EQ(1, group.GetValue(AZ::RPI::ShaderOptionIndex{j}).GetIndex());
                }
                else
                {
                    EXPECT_FALSE(group.GetValue(AZ::RPI::ShaderOptionIndex{j}).IsValid());
                }
            }

            AZStd::bitset<AZ::RPI::ShaderVariantKeyBitCount> expected = allBitsOff;
            expected[targetBit] = true;

            EXPECT_EQ(expected, group.GetShaderVariantId().m_key);
            EXPECT_EQ(expected, group.GetShaderVariantId().m_mask);
        }
    }

    TEST_F(ShaderTests, ShaderOptionGroup_AccessEachBit_AllOtherOptionsTrue)
    {
        AZStd::bitset<AZ::RPI::ShaderVariantKeyBitCount> allBitsOn;
        allBitsOn.set();
        for (size_t i = 0; i < AZ::RPI::ShaderVariantKeyBitCount; ++i)
        {
            EXPECT_TRUE(allBitsOn[i]);
        }

        for (size_t targetBit = 0; targetBit < AZ::RPI::ShaderVariantKeyBitCount; ++targetBit)
        {
            RPI::ShaderOptionGroup group(CreateOptionsLayoutWithAllBools());

            // Set all other bits on
            for (int j = 0; j < AZ::RPI::ShaderVariantKeyBitCount; ++j)
            {
                group.SetValue(AZ::RPI::ShaderOptionIndex{j}, AZ::RPI::ShaderOptionValue{1});
            }

            // Set the target bit off
            group.SetValue(AZ::RPI::ShaderOptionIndex{targetBit}, AZ::RPI::ShaderOptionValue{0});

            for (int j = 0; j < AZ::RPI::ShaderVariantKeyBitCount; ++j)
            {
                if (j == targetBit)
                {
                    EXPECT_TRUE(group.GetValue(AZ::RPI::ShaderOptionIndex{j}).IsValid());
                    EXPECT_EQ(0, group.GetValue(AZ::RPI::ShaderOptionIndex{j}).GetIndex());
                }
                else
                {
                    EXPECT_TRUE(group.GetValue(AZ::RPI::ShaderOptionIndex{j}).IsValid());
                    EXPECT_EQ(1, group.GetValue(AZ::RPI::ShaderOptionIndex{j}).GetIndex());
                }
            }

            AZStd::bitset<AZ::RPI::ShaderVariantKeyBitCount> expected = allBitsOn;
            expected[targetBit] = false;

            EXPECT_EQ(expected, group.GetShaderVariantId().m_key);
            EXPECT_EQ(allBitsOn, group.GetShaderVariantId().m_mask);
        }
    }

    TEST_F(ShaderTests, ShaderOptionGroup_SetAllToDefaultValues)
    {
        using namespace AZ;

        RPI::ShaderOptionGroup group(m_shaderOptionGroupLayoutForAsset);

        EXPECT_FALSE(group.GetValue(Name{"Color"}).IsValid());
        EXPECT_FALSE(group.GetValue(Name{"Quality"}).IsValid());
        EXPECT_FALSE(group.GetValue(Name{"NumberSamples"}).IsValid());
        EXPECT_FALSE(group.GetValue(Name{"Raytracing"}).IsValid());

        group.SetAllToDefaultValues();

        EXPECT_EQ(13, group.GetValue(Name{"Color"}).GetIndex());
        EXPECT_EQ(0,  group.GetValue(Name{"Quality"}).GetIndex());
        EXPECT_EQ(50, group.GetValue(Name{"NumberSamples"}).GetIndex());
        EXPECT_EQ(0,  group.GetValue(Name{"Raytracing"}).GetIndex());
    }

    TEST_F(ShaderTests, ShaderOptionGroup_SetUnspecifiedToDefaultValues)
    {
        using namespace AZ;

        RPI::ShaderOptionGroup group(m_shaderOptionGroupLayoutForAsset);

        EXPECT_FALSE(group.GetValue(Name{"Color"}).IsValid());
        EXPECT_FALSE(group.GetValue(Name{"Quality"}).IsValid());
        EXPECT_FALSE(group.GetValue(Name{"NumberSamples"}).IsValid());
        EXPECT_FALSE(group.GetValue(Name{"Raytracing"}).IsValid());

        group.SetValue(Name{"Color"}, Name("Yellow"));
        group.SetValue(Name{"Raytracing"}, Name("On"));

        group.SetUnspecifiedToDefaultValues();

        EXPECT_EQ(11, group.GetValue(Name{"Color"}).GetIndex());
        EXPECT_EQ(0, group.GetValue(Name{"Quality"}).GetIndex());
        EXPECT_EQ(50, group.GetValue(Name{"NumberSamples"}).GetIndex());
        EXPECT_EQ(1, group.GetValue(Name{"Raytracing"}).GetIndex());
    }

    TEST_F(ShaderTests, ShaderOptionGroup_ToString)
    {
        using namespace AZ;

        RPI::ShaderOptionGroup group(m_shaderOptionGroupLayoutForAsset);

        group.SetValue(Name("Color"), Name("Silver"));      //   7
        group.SetValue(Name("NumberSamples"), Name("50"));  //  50
        group.SetValue(Name("Raytracing"), Name("On"));     //   1

        EXPECT_STREQ("Color=7, Quality=?, NumberSamples=50, Raytracing=1", group.ToString().c_str());
    }


    TEST_F(ShaderTests, ShaderOptionGroupTest_Errors)
    {
        using namespace AZ::RPI;

        Ptr<ShaderOptionGroupLayout> layout = CreateShaderOptionLayout();
        ShaderOptionIndex colorIndex = layout->FindShaderOptionIndex(Name{ "Color" });
        ShaderOptionValue redValue = layout->GetShaderOption(colorIndex).FindValue(Name("Red"));

        RPI::ShaderOptionGroup group(layout);

        // Setting by option index and value index...

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(group.SetValue(ShaderOptionIndex{}, ShaderOptionValue{}));
        EXPECT_FALSE(group.SetValue(ShaderOptionIndex{}, redValue));
        EXPECT_FALSE(group.SetValue(colorIndex, ShaderOptionValue{}));
        AZ_TEST_STOP_TRACE_SUPPRESSION(3);

        EXPECT_TRUE(group.SetValue(colorIndex, redValue));

        // Setting by option name and value index...

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(group.SetValue(Name{ "DoesNotExist" }, ShaderOptionValue{}));
        EXPECT_FALSE(group.SetValue(Name{ "DoesNotExist" }, redValue));
        EXPECT_FALSE(group.SetValue(Name{ "Color" }, ShaderOptionValue{}));
        AZ_TEST_STOP_TRACE_SUPPRESSION(3);

        EXPECT_TRUE(group.SetValue(Name{ "Color" }, redValue));

        // Setting by option index and value name...

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(group.SetValue(ShaderOptionIndex{}, Name{ "DoesNotExist" }));
        EXPECT_FALSE(group.SetValue(ShaderOptionIndex{}, Name{ "Red" }));
        EXPECT_FALSE(group.SetValue(colorIndex, Name{ "DoesNotExist" }));
        AZ_TEST_STOP_TRACE_SUPPRESSION(3);

        EXPECT_TRUE(group.SetValue(colorIndex, Name{ "Red" }));

        // Setting by option name and value name...

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(group.SetValue(Name{ "DoesNotExist" }, Name{ "DoesNotExist" }));
        EXPECT_FALSE(group.SetValue(Name{ "DoesNotExist" }, Name{ "Red" }));
        EXPECT_FALSE(group.SetValue(Name{ "Color" }, Name{ "DoesNotExist" }));
        AZ_TEST_STOP_TRACE_SUPPRESSION(3);

        EXPECT_TRUE(group.SetValue(Name{ "Color" }, Name{ "Red" }));

        // GetValue by option index...

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(group.GetValue(ShaderOptionIndex{}).IsValid());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_TRUE(group.GetValue(colorIndex).IsValid());

        // GetValue by option name...

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(group.GetValue(Name{ "DoesNotExist" }).IsValid());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_TRUE(group.GetValue(Name{ "Color" }).IsValid());

        // Clearing by option index...

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(group.ClearValue(ShaderOptionIndex{}));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_TRUE(group.ClearValue(colorIndex));

        // Clearing by option name...

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(group.ClearValue(Name{ "DoesNotExist" }));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_TRUE(group.ClearValue(Name{ "Color" }));

    }

    TEST_F(ShaderTests, ShaderAsset_Baseline_Test)
    {
        using namespace AZ;

        ValidateShaderAsset(CreateShaderAsset());
    }

    TEST_F(ShaderTests, ShaderAsset_PipelineStateType_VertexImpliesDraw)
    {
        AZ::RPI::ShaderAssetCreator creator;
        BeginCreatingTestShaderAsset(creator, {RHI::ShaderStage::Vertex});
        AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);

        EXPECT_TRUE(shaderAsset);
        EXPECT_EQ(shaderAsset->GetPipelineStateType(), RHI::PipelineStateType::Draw);
    }

    TEST_F(ShaderTests, ShaderAsset_PipelineStateType_ComputeImpliesDispatch)
    {
        AZ::RPI::ShaderAssetCreator creator;
        BeginCreatingTestShaderAsset(creator, {AZ::RHI::ShaderStage::Compute});
        AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);

        EXPECT_TRUE(shaderAsset);
        EXPECT_EQ(shaderAsset->GetPipelineStateType(), RHI::PipelineStateType::Dispatch);
    }

    TEST_F(ShaderTests, ShaderAsset_PipelineStateType_Error_DrawAndDispatch)
    {
        ErrorMessageFinder messageFinder("both Draw functions and Dispatch functions");
        messageFinder.AddExpectedErrorMessage("Invalid root variant");
        messageFinder.AddExpectedErrorMessage("Cannot continue building ShaderAsset because 1 error(s) reported");

        AZ::RPI::ShaderAssetCreator creator;
        BeginCreatingTestShaderAsset(creator,
            {AZ::RHI::ShaderStage::Vertex, AZ::RHI::ShaderStage::Fragment, AZ::RHI::ShaderStage::Compute});

        AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);

        EXPECT_FALSE(shaderAsset);
    }

    TEST_F(ShaderTests, ShaderAsset_Error_FragmentFunctionRequiresVertexFunction)
    {
        ErrorMessageFinder messageFinder("fragment function but no vertex function");
        messageFinder.AddExpectedErrorMessage("Invalid root variant");
        messageFinder.AddExpectedErrorMessage("Cannot continue building ShaderAsset because 1 error(s) reported");

        AZ::RPI::ShaderAssetCreator creator;
        BeginCreatingTestShaderAsset(creator, {AZ::RHI::ShaderStage::Fragment});

        AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);

        messageFinder.CheckExpectedErrorsFound();

        EXPECT_FALSE(shaderAsset);
    }

    TEST_F(ShaderTests, ShaderAsset_Error_GeometryFunctionRequiresVertexFunction)
    {
        ErrorMessageFinder messageFinder("geometry function but no vertex function");
        messageFinder.AddExpectedErrorMessage("Invalid root variant");
        messageFinder.AddExpectedErrorMessage("Cannot continue building ShaderAsset because 1 error(s) reported");

        AZ::RPI::ShaderAssetCreator creator;
        BeginCreatingTestShaderAsset(creator, { AZ::RHI::ShaderStage::Geometry });

        AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);

        messageFinder.CheckExpectedErrorsFound();

        EXPECT_FALSE(shaderAsset);
    }

    TEST_F(ShaderTests, ShaderAsset_Serialize_Test)
    {
        using namespace AZ;

        Data::Asset<RPI::ShaderAsset> shaderAsset = CreateShaderAsset();
        ValidateShaderAsset(shaderAsset);

        RPI::ShaderAssetTester tester(GetSerializeContext());
        tester.SerializeOut(shaderAsset.Get());

        Data::Asset<RPI::ShaderAsset> serializedShaderAsset = tester.SerializeInHelper(Uuid::CreateRandom());
        ValidateShaderAsset(serializedShaderAsset);
    }

    TEST_F(ShaderTests, ShaderAsset_PipelineLayout_Missing_Test)
    {
        using namespace AZ;

        m_pipelineLayoutDescriptor = nullptr;

        AZ_TEST_START_TRACE_SUPPRESSION;
        Data::Asset<RPI::ShaderAsset> shaderAsset = CreateShaderAsset();
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        EXPECT_FALSE(shaderAsset);
    }

    TEST_F(ShaderTests, ShaderAsset_ShaderOptionGroupLayout_Mismatch_Test)
    {
        using namespace AZ;

        const size_t indexToOmit = 0;

        // Creates a shader option group layout assigned to the asset which doesn't match the
        // one assigned to the the variants.

        m_shaderOptionGroupLayoutForAsset = CreateShaderOptionLayout(RHI::Handle<size_t>(indexToOmit));

        AZ_TEST_START_TRACE_SUPPRESSION;
        Data::Asset<RPI::ShaderAsset> shaderAsset = CreateShaderAsset();
        Data::Asset<RPI::ShaderVariantTreeAsset> shaderVariantTreeAsset = CreateShaderVariantTreeAssetForSearch(shaderAsset);
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

        EXPECT_FALSE(shaderVariantTreeAsset);
    }

    TEST_F(ShaderTests, ShaderAsset_DefaultShaderOptions)
    {
        using namespace AZ;

        RPI::ShaderAssetCreator creator;
        BeginCreatingTestShaderAsset(creator);
        // Override two of the default values. The others will maintain the default value from the shader options layout, see SetUp().
        creator.SetShaderOptionDefaultValue(Name{"Quality"}, Name{"Quality::Average"});
        creator.SetShaderOptionDefaultValue(Name{"Raytracing"}, Name{"On"});
        Data::Asset<RPI::ShaderAsset> shaderAssetWithShaderOptionOverrides = EndCreatingTestShaderAsset(creator);

        // These options were overridden
        EXPECT_EQ(3, shaderAssetWithShaderOptionOverrides->GetDefaultShaderOptions().GetValue(Name{"Quality"}).GetIndex());
        EXPECT_EQ(1, shaderAssetWithShaderOptionOverrides->GetDefaultShaderOptions().GetValue(Name{"Raytracing"}).GetIndex());

        // These options maintain their original default values
        EXPECT_EQ(13, shaderAssetWithShaderOptionOverrides->GetDefaultShaderOptions().GetValue(Name{"Color"}).GetIndex());
        EXPECT_EQ(50, shaderAssetWithShaderOptionOverrides->GetDefaultShaderOptions().GetValue(Name{"NumberSamples"}).GetIndex());
    }

    TEST_F(ShaderTests, Shader_Baseline_Test)
    {
        using namespace AZ;


        Data::Instance<RPI::Shader> shader = RPI::Shader::FindOrCreate(CreateShaderAsset());

        ValidateShader(shader);
    }

    TEST_F(ShaderTests, ValidateShaderVariantIdMath)
    {
        RPI::ShaderVariantId           idSmall;
        RPI::ShaderVariantId           idLarge;
        RPI::ShaderVariantIdComparator idComparator;

        idSmall.m_mask = RPI::ShaderVariantKey(15);
        idLarge.m_mask = RPI::ShaderVariantKey(31);
        idSmall.m_key = RPI::ShaderVariantKey(15);
        idLarge.m_key = RPI::ShaderVariantKey(31);

        EXPECT_TRUE(idComparator(idSmall, idLarge));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idSmall, idLarge), -1);
        EXPECT_FALSE(idComparator(idLarge, idSmall));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idLarge, idSmall), 1);

        // The mask has precedence so the evaluation is the same as above
        idSmall.m_key = RPI::ShaderVariantKey(31);
        idLarge.m_key = RPI::ShaderVariantKey(15);
        EXPECT_TRUE(idComparator(idSmall, idLarge));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idSmall, idLarge), -1);
        EXPECT_FALSE(idComparator(idLarge, idSmall));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idLarge, idSmall), 1);

        // The mask has precedence so the evaluation is the same as above
        idSmall.m_key = RPI::ShaderVariantKey(0);
        idLarge.m_key = RPI::ShaderVariantKey(0);
        EXPECT_TRUE(idComparator(idSmall, idLarge));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idSmall, idLarge), -1);
        EXPECT_FALSE(idComparator(idLarge, idSmall));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idLarge, idSmall), 1);

        // The mask has precedence so the evaluation is the same as above
        idSmall.m_key = RPI::ShaderVariantKey(63);
        idLarge.m_key = RPI::ShaderVariantKey(63);
        EXPECT_TRUE(idComparator(idSmall, idLarge));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idSmall, idLarge), -1);
        EXPECT_FALSE(idComparator(idLarge, idSmall));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idLarge, idSmall), 1);

        // In the case where the mask are equal, the id's key should be used
        idSmall.m_mask = RPI::ShaderVariantKey(31);
        idLarge.m_mask = RPI::ShaderVariantKey(31);
        idSmall.m_key = RPI::ShaderVariantKey(6);
        idLarge.m_key = RPI::ShaderVariantKey(20);

        EXPECT_TRUE(idComparator(idSmall, idLarge));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idSmall, idLarge), -1);
        EXPECT_FALSE(idComparator(idLarge, idSmall));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idLarge, idSmall), 1);

        // The variant id is the same
        idSmall.m_mask = RPI::ShaderVariantKey(31);
        idLarge.m_mask = RPI::ShaderVariantKey(31);
        idSmall.m_key = RPI::ShaderVariantKey(15);
        idLarge.m_key = RPI::ShaderVariantKey(15);

        EXPECT_FALSE(idComparator(idSmall, idLarge));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idSmall, idLarge), 0);
        EXPECT_FALSE(idComparator(idLarge, idSmall));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idLarge, idSmall), 0);

        // The variant id is the same
        idSmall.m_mask = RPI::ShaderVariantKey(0);
        idLarge.m_mask = RPI::ShaderVariantKey(0);

        EXPECT_FALSE(idComparator(idSmall, idLarge));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idSmall, idLarge), 0);
        EXPECT_FALSE(idComparator(idLarge, idSmall));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idLarge, idSmall), 0);

        // If the mask is 0, the key has insignificant bits, the variant id is the same
        idSmall.m_mask = RPI::ShaderVariantKey(0);
        idLarge.m_mask = RPI::ShaderVariantKey(0);
        idSmall.m_key = RPI::ShaderVariantKey(31);
        idLarge.m_key = RPI::ShaderVariantKey(15);

        EXPECT_FALSE(idComparator(idSmall, idLarge));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idSmall, idLarge), 0);
        EXPECT_FALSE(idComparator(idLarge, idSmall));
        EXPECT_EQ(RPI::ShaderVariantIdComparator::Compare(idLarge, idSmall), 0);

    }

    TEST_F(ShaderTests, ValidateShaderVariantKeyFallbackPacking)
    {
        AZStd::vector<RPI::ShaderOptionValuePair> idList0;
        idList0.push_back({ Name("Black"),      RPI::ShaderOptionValue(0) }); // 1+ bit
        idList0.push_back({ Name("Maroon"),     RPI::ShaderOptionValue(1) }); // ...
        idList0.push_back({ Name("Green"),      RPI::ShaderOptionValue(2) }); // 2+ bits
        idList0.push_back({ Name("Olive"),      RPI::ShaderOptionValue(3) }); // ...
        idList0.push_back({ Name("Navy"),       RPI::ShaderOptionValue(4) }); // 3+ bits
        idList0.push_back({ Name("Purple"),     RPI::ShaderOptionValue(5) }); // ...
        idList0.push_back({ Name("Teal"),       RPI::ShaderOptionValue(6) }); // ...
        idList0.push_back({ Name("Silver"),     RPI::ShaderOptionValue(7) }); // ...
        idList0.push_back({ Name("Gray"),       RPI::ShaderOptionValue(8) }); // 4+ bits
        idList0.push_back({ Name("Red"),        RPI::ShaderOptionValue(9) }); // ...
        idList0.push_back({ Name("Lime"),       RPI::ShaderOptionValue(10) }); // ...
        idList0.push_back({ Name("Yellow"),     RPI::ShaderOptionValue(11) }); // ...
        idList0.push_back({ Name("Blue"),       RPI::ShaderOptionValue(12) }); // ...
        idList0.push_back({ Name("Fuchsia"),    RPI::ShaderOptionValue(13) }); // ...
        idList0.push_back({ Name("Cyan"),       RPI::ShaderOptionValue(14) }); // ...
        idList0.push_back({ Name("White"),      RPI::ShaderOptionValue(15) }); // ...
        idList0.push_back({ Name("Beige"),      RPI::ShaderOptionValue(16) }); // 5 bits!!

        // Six descriptors with 5 bits each are 30 bits, but AZSLc will pack them within 32-bit boundaries, so
        // every six descriptors will end up wasting 2 bits of register space.
        // This test checks for values up to 256 bits
        uint32_t bitOffset = 0;
        uint32_t order = 0;
        constexpr uint32_t descriptorsPerElement = 6;
        constexpr uint32_t numberOfElements = RPI::ShaderVariantKeyBitCount / RPI::ShaderElementBitSize;
        RPI::ShaderOptionDescriptor descriptor[numberOfElements * descriptorsPerElement];
        RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();

        for (int i = 0; i < numberOfElements * descriptorsPerElement; i++)
        {
            std::stringstream ss;
            ss << "Color" << i;
            descriptor[i] = RPI::ShaderOptionDescriptor{ Name(ss.str().c_str()),
                                                         RPI::ShaderOptionType::Enumeration,
                                                         bitOffset,
                                                         order++,
                                                         idList0,
                                                         Name("Fuchsia") };

            shaderOptionGroupLayout->AddShaderOption(descriptor[i]);

            EXPECT_EQ(descriptor[i].GetBitCount(), 5);

            bitOffset = descriptor[i].GetBitOffset() + descriptor[i].GetBitCount();

            // This hack up-aligns the bit offset to match the AZSLc behavior
            // (AZSLc respects a 32-bit boundary for any options used)
            // It doesn't matter for the test itself since we read raw data
            if (i % descriptorsPerElement == (descriptorsPerElement - 1))
            {
                bitOffset += 2;
            }
        }
        shaderOptionGroupLayout->Finalize();

        // Create and test a few ShaderOptionGroup-s
        // This simple test matches the expected padding for AZSLc and should only be updated
        // if AZSLc.exe changes the shader variant key fallback mask.
        auto shaderOptionGroup = RPI::ShaderOptionGroup(shaderOptionGroupLayout);

        // ShaderVariantKey is 32 or more bits
        if constexpr (numberOfElements >= 1)
        {
            shaderOptionGroup.SetValue(AZ::Name("Color0"), AZ::Name("Beige"));   //   16
            shaderOptionGroup.SetValue(AZ::Name("Color1"), AZ::Name("Olive"));   //   3
            shaderOptionGroup.SetValue(AZ::Name("Color2"), AZ::Name("Navy"));    //   4
            shaderOptionGroup.SetValue(AZ::Name("Color3"), AZ::Name("Teal"));    //   6
            shaderOptionGroup.SetValue(AZ::Name("Color4"), AZ::Name("Lime"));    //  10
            shaderOptionGroup.SetValue(AZ::Name("Color5"), AZ::Name("Fuchsia")); //  13
        }

        // ShaderVariantKey is 64 or more bits
        if constexpr (numberOfElements >= 2)
        {
            shaderOptionGroup.SetValue(AZ::Name("Color6"), AZ::Name("Olive"));    //   3
            shaderOptionGroup.SetValue(AZ::Name("Color7"), AZ::Name("Beige"));    //   16
            shaderOptionGroup.SetValue(AZ::Name("Color8"), AZ::Name("Navy"));     //   4
            shaderOptionGroup.SetValue(AZ::Name("Color9"), AZ::Name("Teal"));     //   6
            shaderOptionGroup.SetValue(AZ::Name("Color10"), AZ::Name("Lime"));    //  10
            shaderOptionGroup.SetValue(AZ::Name("Color11"), AZ::Name("Fuchsia")); //  13
        }

        // ShaderVariantKey is 96 or more bits
        if constexpr (numberOfElements >= 3)
        {
            shaderOptionGroup.SetValue(AZ::Name("Color12"), AZ::Name("Navy"));    //   4
            shaderOptionGroup.SetValue(AZ::Name("Color13"), AZ::Name("Beige"));   //   16
            shaderOptionGroup.SetValue(AZ::Name("Color14"), AZ::Name("Olive"));   //   3
            shaderOptionGroup.SetValue(AZ::Name("Color15"), AZ::Name("Teal"));    //   6
            shaderOptionGroup.SetValue(AZ::Name("Color16"), AZ::Name("Lime"));    //  10
            shaderOptionGroup.SetValue(AZ::Name("Color17"), AZ::Name("Fuchsia")); //  13
        }

        // ShaderVariantKey is 128 or more bits
        if constexpr (numberOfElements >= 4)
        {
            shaderOptionGroup.SetValue(AZ::Name("Color18"), AZ::Name("Teal"));    //   6
            shaderOptionGroup.SetValue(AZ::Name("Color19"), AZ::Name("Beige"));   //   16
            shaderOptionGroup.SetValue(AZ::Name("Color20"), AZ::Name("Olive"));   //   3
            shaderOptionGroup.SetValue(AZ::Name("Color21"), AZ::Name("Navy"));    //   4
            shaderOptionGroup.SetValue(AZ::Name("Color22"), AZ::Name("Lime"));    //  10
            shaderOptionGroup.SetValue(AZ::Name("Color23"), AZ::Name("Fuchsia")); //  13
        }

        // ShaderVariantKey is 160 or more bits
        if constexpr (numberOfElements >= 5)
        {
            shaderOptionGroup.SetValue(AZ::Name("Color24"), AZ::Name("Navy"));    //   4
            shaderOptionGroup.SetValue(AZ::Name("Color25"), AZ::Name("Teal"));    //   6
            shaderOptionGroup.SetValue(AZ::Name("Color26"), AZ::Name("Lime"));    //  10
            shaderOptionGroup.SetValue(AZ::Name("Color27"), AZ::Name("Fuchsia")); //  13
            shaderOptionGroup.SetValue(AZ::Name("Color28"), AZ::Name("Beige"));   //   16
            shaderOptionGroup.SetValue(AZ::Name("Color29"), AZ::Name("Olive"));   //   3
        }

        // ShaderVariantKey is 192 or more bits
        if constexpr (numberOfElements >= 6)
        {
            shaderOptionGroup.SetValue(AZ::Name("Color30"), AZ::Name("Teal"));    //   6
            shaderOptionGroup.SetValue(AZ::Name("Color31"), AZ::Name("Lime"));    //  10
            shaderOptionGroup.SetValue(AZ::Name("Color32"), AZ::Name("Fuchsia")); //  13
            shaderOptionGroup.SetValue(AZ::Name("Color33"), AZ::Name("Beige"));   //   16
            shaderOptionGroup.SetValue(AZ::Name("Color34"), AZ::Name("Olive"));   //   3
            shaderOptionGroup.SetValue(AZ::Name("Color35"), AZ::Name("Navy"));    //   4
        }

        // ShaderVariantKey is 224 or more bits
        if constexpr (numberOfElements >= 7)
        {
            shaderOptionGroup.SetValue(AZ::Name("Color36"), AZ::Name("Lime"));    //  10
            shaderOptionGroup.SetValue(AZ::Name("Color37"), AZ::Name("Fuchsia")); //  13
            shaderOptionGroup.SetValue(AZ::Name("Color38"), AZ::Name("Beige"));   //   16
            shaderOptionGroup.SetValue(AZ::Name("Color39"), AZ::Name("Olive"));   //   3
            shaderOptionGroup.SetValue(AZ::Name("Color40"), AZ::Name("Navy"));    //   4
            shaderOptionGroup.SetValue(AZ::Name("Color41"), AZ::Name("Teal"));    //   6
        }

        // ShaderVariantKey is 256 or more bits
        if constexpr (numberOfElements >= 8)
        {
            shaderOptionGroup.SetValue(AZ::Name("Color42"), AZ::Name("Fuchsia")); //  13
            shaderOptionGroup.SetValue(AZ::Name("Color43"), AZ::Name("Beige"));   //   16
            shaderOptionGroup.SetValue(AZ::Name("Color44"), AZ::Name("Olive"));   //   3
            shaderOptionGroup.SetValue(AZ::Name("Color45"), AZ::Name("Navy"));    //   4
            shaderOptionGroup.SetValue(AZ::Name("Color46"), AZ::Name("Teal"));    //   6
            shaderOptionGroup.SetValue(AZ::Name("Color47"), AZ::Name("Lime"));    //  10
        }

        uint32_t fallbackValue[RPI::ShaderVariantKeyAlignedBitCount / RPI::ShaderElementBitSize];
        memcpy(fallbackValue, shaderOptionGroup.GetShaderVariantId().m_key.data(), RPI::ShaderVariantKeyBitCount / 8);

        if constexpr (numberOfElements > 0)
        {
            EXPECT_EQ(fallbackValue[0], 0x1aa31070);
        }

        if constexpr (numberOfElements > 1)
        {
            EXPECT_EQ(fallbackValue[1], 0x1aa31203);
        }

        if constexpr (numberOfElements > 2)
        {
            EXPECT_EQ(fallbackValue[2], 0x1aa30e04);
        }

        if constexpr (numberOfElements > 3)
        {
            EXPECT_EQ(fallbackValue[3], 0x1aa20e06);
        }

        if constexpr (numberOfElements > 4)
        {
            EXPECT_EQ(fallbackValue[4], 0x0706a8c4);
        }

        if constexpr (numberOfElements > 5)
        {
            EXPECT_EQ(fallbackValue[5], 0x08383546);
        }

        if constexpr (numberOfElements > 6)
        {
            EXPECT_EQ(fallbackValue[6], 0x0c41c1aa);
        }

        if constexpr (numberOfElements > 7)
        {
            EXPECT_EQ(fallbackValue[7], 0x14620e0d);
        }
    }

    TEST_F(ShaderTests, ShaderAsset_ValidateSearch)
    {
        using namespace AZ;
        using namespace AZ::RPI;

        auto shaderAsset = CreateShaderAsset();
        auto shaderVariantTreeAsset = CreateShaderVariantTreeAssetForSearch(shaderAsset);

        // We expect the following composition:
        // Index 0 - []
        // Index 1 - [Fuchsia]
        // Index 2 - [Fuchsia, Quality::Auto]
        // Index 3 - [Fuchsia, Quality::Auto, 50]
        // Index 4 - [Fuchsia, Quality::Auto, 50, Off]
        // Index 5 - [Fuchsia, Quality::Auto, 50, On]
        // Index 6 - [Teal]
        // Index 7 - [Teal, Quality::Sublime]

        // Let's search it!
        RPI::ShaderOptionGroup shaderOptionGroup(m_shaderOptionGroupLayoutForVariants);

        const uint32_t stableId0 = 0;
        const uint32_t stableId1 = 1;
        const uint32_t stableId2 = 2;
        const uint32_t stableId3 = 3;
        const uint32_t stableId4 = 4;
        const uint32_t stableId5 = 5;
        const uint32_t stableId6 = 6;
        const uint32_t stableId7 = 7;

        // Index 0 - []
        const auto& result0 = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_TRUE(result0.IsRoot());
        EXPECT_FALSE(result0.IsFullyBaked());
        EXPECT_EQ(result0.GetStableId().GetIndex(), stableId0);

        // Index 1 - [Fuchsia]
        shaderOptionGroup.SetValue(Name("Color"), Name("Fuchsia"));
        const auto& result1 = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(result1.IsRoot());
        EXPECT_FALSE(result1.IsFullyBaked());
        EXPECT_EQ(result1.GetStableId().GetIndex(), stableId1);

        // Index 2 - [Fuchsia, Quality::Auto]
        shaderOptionGroup.SetValue(Name("Quality"), Name("Quality::Auto"));
        const auto& result2 = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(result2.IsRoot());
        EXPECT_FALSE(result2.IsFullyBaked());
        EXPECT_EQ(result2.GetStableId().GetIndex(), stableId2);

        // Index 3 - [Fuchsia, Quality::Auto, 50]
        shaderOptionGroup.SetValue(Name("NumberSamples"), Name("50"));
        const auto& result3 = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(result3.IsRoot());
        EXPECT_FALSE(result3.IsFullyBaked());
        EXPECT_EQ(result3.GetStableId().GetIndex(), stableId3);

        // Index 4 - [Fuchsia, Quality::Auto, 50, Off]
        shaderOptionGroup.SetValue(Name("Raytracing"), Name("Off"));
        const auto& result4 = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(result4.IsRoot());
        EXPECT_TRUE(result4.IsFullyBaked());
        EXPECT_EQ(result4.GetStableId().GetIndex(), stableId4);

        // Index 5 - [Fuchsia, Quality::Auto, 50, On]
        shaderOptionGroup.SetValue(Name("Raytracing"), Name("On"));
        const auto& result5 = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(result5.IsRoot());
        EXPECT_TRUE(result5.IsFullyBaked());
        EXPECT_EQ(result5.GetStableId().GetIndex(), stableId5);

        shaderOptionGroup.Clear();

        // Index 6 - [Teal]
        shaderOptionGroup.SetValue(Name("Color"), Name("Teal"));
        const auto& result6 = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(result6.IsRoot());
        EXPECT_FALSE(result6.IsFullyBaked());
        EXPECT_EQ(result6.GetStableId().GetIndex(), stableId6);

        // Index 7 - [Teal, Quality::Sublime]
        shaderOptionGroup.SetValue(Name("Quality"), Name("Quality::Sublime"));
        const auto& result7 = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(result7.IsRoot());
        EXPECT_FALSE(result7.IsFullyBaked());
        EXPECT_EQ(result7.GetStableId().GetIndex(), stableId7);

        /*
           All searches so far found exactly the node we were looking for
           The next couple of searches will not find the requested node
            and will instead default to its parent, up the tree to the root

           []                       [Root]
                                    /    \
           [Color]              [Teal]  [Fuchsia]
                                  /        \
           [Quality]          [Sublime]   [Auto]
                                            /
           [NumberSamples]                [50]
                                          /  \
           [Raytracing]                [On]  [Off]
        */

        // ----------------------------------------
        // [Quality::Poor]
        shaderOptionGroup.Clear();
        shaderOptionGroup.SetValue(Name("Color"), Name("Fuchsia"));
        shaderOptionGroup.SetValue(Name("Quality"), Name("Quality::Poor"));

        // This node doesn't exist, but setting the quality forced Color to its default value, so we expect to get:
        // Index 1 - [Fuchsia]
        const auto& result8 = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(result8.IsRoot());
        EXPECT_FALSE(result8.IsFullyBaked());
        EXPECT_EQ(result8.GetStableId().GetIndex(), stableId1);

        // ----------------------------------------
        // [Teal, Quality::Poor]
        shaderOptionGroup.Clear();
        shaderOptionGroup.SetValue(Name("Quality"), Name("Quality::Poor"));
        shaderOptionGroup.SetValue(Name("Color"), Name("Teal"));

        // This node doesn't exist, but we have set both Color and Quality so we expect to get:
        // Index 6 - [Teal]
        const auto& result9 = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(result9.IsRoot());
        EXPECT_FALSE(result9.IsFullyBaked());
        EXPECT_EQ(result9.GetStableId().GetIndex(), stableId6);

        // ----------------------------------------
        // [Navy, Quality::Good]
        shaderOptionGroup.Clear();
        shaderOptionGroup.SetValue(Name("Quality"), Name("Quality::Good"));
        shaderOptionGroup.SetValue(Name("Color"), Name("Navy"));

        // This node doesn't exist (Good Navy), its parent (Navy) doesn't exist either so we expect to get the root:
        // Index 0 - []
        const auto& resultA = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_TRUE(resultA.IsRoot());
        EXPECT_FALSE(resultA.IsFullyBaked());
        EXPECT_EQ(resultA.GetStableId().GetIndex(), stableId0);

        // ----------------------------------------
        // [Teal, Quality::Sublime, 50, Off] - Test 1/3
        shaderOptionGroup.Clear();
        shaderOptionGroup.SetValue(Name("Color"), Name("Teal"));
        shaderOptionGroup.SetValue(Name("Quality"), Name("Quality::Sublime"));
        shaderOptionGroup.SetValue(Name("NumberSamples"), Name("50"));
        shaderOptionGroup.SetValue(Name("Raytracing"), Name("Off"));

        // No specialized nodes exist under (Teal, Sublime) so we expect to get that:
        // Index 7 - [Teal, Quality::Sublime]
        const auto& resultB = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(resultB.IsRoot());
        EXPECT_FALSE(resultB.IsFullyBaked());
        EXPECT_EQ(resultB.GetStableId().GetIndex(), stableId7);

        // ----------------------------------------
        // [Teal, Quality::Sublime, 50, On] - Test 2/3
        shaderOptionGroup.Clear();
        shaderOptionGroup.SetValue(Name("Color"), Name("Teal"));
        shaderOptionGroup.SetValue(Name("Quality"), Name("Quality::Sublime"));
        shaderOptionGroup.SetValue(Name("NumberSamples"), Name("50"));
        shaderOptionGroup.SetValue(Name("Raytracing"), Name("On"));

        // No specialized nodes exist under (Teal, Sublime) so we expect to get that:
        // Index 7 - [Teal, Quality::Sublime]
        const auto& resultC = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(resultC.IsRoot());
        EXPECT_FALSE(resultC.IsFullyBaked());
        EXPECT_EQ(resultC.GetStableId().GetIndex(), stableId7);

        // ----------------------------------------
        // [Teal, Quality::Sublime, 150] - Test 3/3
        shaderOptionGroup.Clear();
        shaderOptionGroup.SetValue(Name("Color"), Name("Teal"));
        shaderOptionGroup.SetValue(Name("Quality"), Name("Quality::Sublime"));
        shaderOptionGroup.SetValue(Name("NumberSamples"), Name("150"));

        // No specialized nodes exist under (Teal, Sublime) so we expect to get that:
        // Index 7 - [Teal, Quality::Sublime]
        const auto& resultD = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(resultD.IsRoot());
        EXPECT_FALSE(resultD.IsFullyBaked());
        EXPECT_EQ(resultD.GetStableId().GetIndex(), stableId7);

        // ----------------------------------------
        // [120]
        shaderOptionGroup.Clear();
        shaderOptionGroup.SetValue(Name("Color"), Name("Fuchsia"));
        shaderOptionGroup.SetValue(Name("Quality"), Name("Quality::Auto"));
        shaderOptionGroup.SetValue(Name("NumberSamples"), Name("120"));

        // The node (Fuchsia, Auto, 120) doesn't exist - note that the higher order options assume their default values. We get:
        // Index 2 - [Fuchsia, Quality::Auto]
        const auto& resultE = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(resultE.IsRoot());
        EXPECT_FALSE(resultE.IsFullyBaked());
        EXPECT_EQ(resultE.GetStableId().GetIndex(), stableId2);

        // ----------------------------------------
        // [50]
        shaderOptionGroup.Clear();
        shaderOptionGroup.SetValue(Name("Color"), Name("Fuchsia"));
        shaderOptionGroup.SetValue(Name("Quality"), Name("Quality::Auto"));
        shaderOptionGroup.SetValue(Name("NumberSamples"), Name("50"));

        // ----------------------------------------

        shaderOptionGroup.SetValue(Name("Raytracing"), Name("Off"));

        // Index 4 - [Fuchsia, Quality::Auto, 50, Off]
        const auto& resultF = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(resultF.IsRoot());
        EXPECT_TRUE(resultF.IsFullyBaked());
        EXPECT_EQ(resultF.GetStableId().GetIndex(), stableId4);

        shaderOptionGroup.SetValue(Name("Raytracing"), Name("On"));

        // Index 5 - [Fuchsia, Quality::Auto, 50, On]
        const auto& resultG = shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderOptionGroup.GetShaderVariantId());
        EXPECT_FALSE(resultG.IsRoot());
        EXPECT_TRUE(resultG.IsFullyBaked());
        EXPECT_EQ(resultG.GetStableId().GetIndex(), stableId5);
    }

    TEST_F(ShaderTests, ShaderAsset_SpecializationConstants)
    {
        {
            AZ::RPI::ShaderAssetCreator creator;
            BeginCreatingTestShaderAsset(creator, { RHI::ShaderStage::Compute }, SpecializationType::None);
            AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);
            EXPECT_FALSE(shaderAsset->UseSpecializationConstants());
            EXPECT_FALSE(shaderAsset->IsFullySpecialized());
        }

        {
            AZ::RPI::ShaderAssetCreator creator;
            BeginCreatingTestShaderAsset(creator, { RHI::ShaderStage::Compute }, SpecializationType::Partial);
            AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);
            EXPECT_TRUE(shaderAsset->UseSpecializationConstants());
            EXPECT_FALSE(shaderAsset->IsFullySpecialized());
        }

        {
            AZ::RPI::ShaderAssetCreator creator;
            BeginCreatingTestShaderAsset(creator, { RHI::ShaderStage::Compute }, SpecializationType::Full);
            AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);
            EXPECT_TRUE(shaderAsset->UseSpecializationConstants());
            EXPECT_TRUE(shaderAsset->IsFullySpecialized());
        }

        m_shaderOptionGroupLayoutForAssetFullSpecialization = m_shaderOptionGroupLayoutForAsset;
        {
            AZ::RPI::ShaderAssetCreator creator;
            BeginCreatingTestShaderAsset(creator, { RHI::ShaderStage::Compute }, SpecializationType::Full);
            AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);
            EXPECT_FALSE(shaderAsset->UseSpecializationConstants());
            EXPECT_FALSE(shaderAsset->IsFullySpecialized());
        }
    }

    TEST_F(ShaderTests, ShaderVariantAsset_IsFullyBaked)
    {
        using namespace AZ;
        using namespace AZ::RPI;

        ShaderOptionGroup shaderOptions{m_shaderOptionGroupLayoutForAsset};

        Data::Asset<ShaderVariantAsset> shaderVariantAsset;

        shaderVariantAsset = CreateTestShaderVariantAsset(shaderOptions.GetShaderVariantId(), RPI::ShaderVariantStableId{0}, false);
        EXPECT_FALSE(shaderVariantAsset->IsFullyBaked());
        EXPECT_FALSE(ShaderOptionGroup(m_shaderOptionGroupLayoutForAsset, shaderVariantAsset->GetShaderVariantId()).IsFullySpecified());

        shaderOptions.SetValue(AZ::Name{"Color"}, AZ::Name{"Yellow"});
        shaderOptions.SetValue(AZ::Name{"Quality"}, AZ::Name{"Quality::Average"});
        shaderOptions.SetValue(AZ::Name{"NumberSamples"}, AZ::Name{"100"});
        shaderOptions.SetValue(AZ::Name{"Raytracing"}, AZ::Name{"On"});
        shaderVariantAsset = CreateTestShaderVariantAsset(shaderOptions.GetShaderVariantId(), RPI::ShaderVariantStableId{0}, true);
        EXPECT_TRUE(shaderVariantAsset->IsFullyBaked());
        EXPECT_TRUE(ShaderOptionGroup(m_shaderOptionGroupLayoutForAsset, shaderVariantAsset->GetShaderVariantId()).IsFullySpecified());

        shaderOptions.ClearValue(AZ::Name{"NumberSamples"});
        shaderVariantAsset = CreateTestShaderVariantAsset(shaderOptions.GetShaderVariantId(), RPI::ShaderVariantStableId{0}, false);
        EXPECT_FALSE(shaderVariantAsset->IsFullyBaked());
        EXPECT_FALSE(ShaderOptionGroup(m_shaderOptionGroupLayoutForAsset, shaderVariantAsset->GetShaderVariantId()).IsFullySpecified());
    }

    TEST_F(ShaderTests, ShaderVariantAsset_IsFullySpecialized)
    {
        using namespace AZ;
        using namespace AZ::RPI;

         {
            AZ::RPI::ShaderAssetCreator creator;
            BeginCreatingTestShaderAsset(creator, { RHI::ShaderStage::Compute }, SpecializationType::None);
            AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);
            Data::Instance<RPI::Shader> shader = RPI::Shader::FindOrCreate(shaderAsset);
            const RPI::ShaderVariant& rootShaderVariant = shader->GetVariant(RPI::ShaderVariantStableId{ 0 });
            EXPECT_TRUE(rootShaderVariant.UseKeyFallback());
            EXPECT_FALSE(rootShaderVariant.IsFullySpecialized());
            EXPECT_FALSE(rootShaderVariant.UseSpecializationConstants());
         }

         {
             AZ::RPI::ShaderAssetCreator creator;
             BeginCreatingTestShaderAsset(creator, { RHI::ShaderStage::Compute }, SpecializationType::Partial);
             AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);
             Data::Instance<RPI::Shader> shader = RPI::Shader::FindOrCreate(shaderAsset);
             const RPI::ShaderVariant& rootShaderVariant = shader->GetVariant(RPI::ShaderVariantStableId{ 0 });
             EXPECT_TRUE(rootShaderVariant.UseKeyFallback());
             EXPECT_FALSE(rootShaderVariant.IsFullySpecialized());
             EXPECT_TRUE(rootShaderVariant.UseSpecializationConstants());
         }

         {
             AZ::RPI::ShaderAssetCreator creator;
             BeginCreatingTestShaderAsset(creator, { RHI::ShaderStage::Compute }, SpecializationType::Full);
             AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = EndCreatingTestShaderAsset(creator);
             Data::Instance<RPI::Shader> shader = RPI::Shader::FindOrCreate(shaderAsset);
             const RPI::ShaderVariant& rootShaderVariant = shader->GetVariant(RPI::ShaderVariantStableId{ 0 });
             EXPECT_FALSE(rootShaderVariant.UseKeyFallback());
             EXPECT_TRUE(rootShaderVariant.IsFullySpecialized());
             EXPECT_TRUE(rootShaderVariant.UseSpecializationConstants());
         }
    }
}

