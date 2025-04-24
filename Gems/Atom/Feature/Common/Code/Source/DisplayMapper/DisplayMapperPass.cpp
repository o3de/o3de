/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DisplayMapper/DisplayMapperPass.h>
#include <ACES/Aces.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DisplayMapperPass> DisplayMapperPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DisplayMapperPass> pass = aznew DisplayMapperPass(descriptor);
            return pass;
        }

        DisplayMapperPass::DisplayMapperPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        {
            const DisplayMapperPassData* passData = RPI::PassUtils::GetPassData<DisplayMapperPassData>(descriptor);
            if (passData != nullptr)
            {
                m_defaultDisplayMapperConfigurationDescriptor = passData->m_config;
                m_mergeLdrGradingLut = passData->m_mergeLdrGradingLut;
                m_outputTransformOverrideShader = passData->m_outputTransformOverride;
            }
            else
            {
                AcesDisplayMapperFeatureProcessor::GetDefaultDisplayMapperConfiguration(m_defaultDisplayMapperConfigurationDescriptor);
            }

            UpdateDisplayMapperConfiguration();
        }

        void DisplayMapperPass::ConfigureDisplayParameters()
        {
            // [GFX TODO] [ATOM-2450] Logic determine the type of display attached and use it to drive the
            // display mapper parameters.
            if (m_pipelineOutput && m_pipelineOutput->GetAttachment())
            {
                m_displayBufferFormat = m_pipelineOutput->GetAttachment()->m_descriptor.m_image.m_format;
            }

            if (m_displayBufferFormat != RHI::Format::Unknown)
            {
                if (m_acesOutputTransformPass)
                {
                    m_acesOutputTransformPass->SetAcesParameterOverrides(m_displayMapperConfigurationDescriptor.m_acesParameterOverrides);
                    m_acesOutputTransformPass->SetDisplayBufferFormat(m_displayBufferFormat);
                }
                if (m_bakeAcesOutputTransformLutPass)
                {
                    m_bakeAcesOutputTransformLutPass->SetDisplayBufferFormat(m_displayBufferFormat);
                    AZ_Assert(m_acesOutputTransformLutPass != nullptr, "AcesOutputTransformLutPass should be created when baking ACES LUTs.");
                    m_acesOutputTransformLutPass->SetShaperParams(m_bakeAcesOutputTransformLutPass->GetShaperParams());
                }
                if (m_outputTransformPass)
                {
                    if (m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::Reinhard ||
                        m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::AcesFitted ||
                        m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::AcesFilmic)
                    {
                        // When using Reinhard tonemapper, a gamma of 2.2 for the transfer function is used for LDR display,
                        // and PQ is used for HDR.
                        if (m_displayBufferFormat == RHI::Format::R8G8B8A8_UNORM ||
                            m_displayBufferFormat == RHI::Format::B8G8R8A8_UNORM)
                        {
                            m_outputTransformPass->SetTransferFunctionType(TransferFunctionType::Gamma22);
                        }
                        else
                        {
                            m_outputTransformPass->SetTransferFunctionType(TransferFunctionType::PerceptualQuantizer);
                        }
                    }
                    else if (m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::Filmic)
                    {
                        m_outputTransformPass->SetTransferFunctionType(TransferFunctionType::None);
                    }

                    m_outputTransformPass->SetDisplayBufferFormat(m_displayBufferFormat);
                }
            }
        }

        void DisplayMapperPass::BuildInternal()
        {
            const Name outputName = Name{ "Output" };
            Name inputPass = Name{ "Parent" };
            Name inputPassAttachment = Name{ "Input" };

            if (m_acesOutputTransformPass)
            {
                m_acesOutputTransformPass->SetInputReferencePassName(inputPass);
                m_acesOutputTransformPass->SetInputReferenceAttachmentName(inputPassAttachment);
                inputPass = m_acesOutputTransformPassName;
                inputPassAttachment = outputName;
            }
            else if (m_acesOutputTransformLutPass)
            {
                m_acesOutputTransformLutPass->SetInputReferencePassName(inputPass);
                m_acesOutputTransformLutPass->SetInputReferenceAttachmentName(inputPassAttachment);
                inputPass = m_acesOutputTransformLutPassName;
                inputPassAttachment = outputName;
            }
            else if (m_displayMapperPassthroughPass)
            {
                m_displayMapperPassthroughPass->SetInputReferencePassName(inputPass);
                m_displayMapperPassthroughPass->SetInputReferenceAttachmentName(inputPassAttachment);
                inputPass = m_displayMapperPassthroughPassName;
                inputPassAttachment = outputName;
            }
            else if (m_displayMapperSRGBPass)
            {
                m_displayMapperSRGBPass->SetInputReferencePassName(inputPass);
                m_displayMapperSRGBPass->SetInputReferenceAttachmentName(inputPassAttachment);
                inputPass = m_displayMapperSRGBPassName;
                inputPassAttachment = outputName;
            }
            else if (m_outputTransformPass)
            {
                m_outputTransformPass->SetInputReferencePassName(inputPass);
                m_outputTransformPass->SetInputReferenceAttachmentName(inputPassAttachment);
                inputPass = m_outputTransformPassName;
                inputPassAttachment = outputName;
            }

            if (m_ldrGradingLookupTablePass)
            {
                m_ldrGradingLookupTablePass->SetInputReferencePassName(inputPass);
                m_ldrGradingLookupTablePass->SetInputReferenceAttachmentName(inputPassAttachment);
            }

            m_pipelineOutput = FindAttachmentBinding(Name("PipelineOutput"));

            ParentPass::BuildInternal();
        }

        void DisplayMapperPass::InitializeInternal()
        {
            // Force update on bindings because children of display mapper pass have their outputs connect to
            // their parent's output, which is a non-conventional and non-standard workflow. Parent outputs are
            // updated after child outputs, so post-build the child outputs do not yet point to the attachments on
            // the parent bindings they are connected to. Forcing this refresh sets the attachment on the child output.
            for (const RPI::Ptr<Pass>& child : m_children)
            {
                child->UpdateConnectedBindings();
            }

            RPI::ParentPass::InitializeInternal();
        }

        void DisplayMapperPass::FrameBeginInternal(FramePrepareParams params)
        {
            ConfigureDisplayParameters();
            ParentPass::FrameBeginInternal(params);
        }

        void DisplayMapperPass::FrameEndInternal()
        {
            UpdateDisplayMapperConfiguration();
            ParentPass::FrameEndInternal();
        }

        void DisplayMapperPass::CreateChildPassesInternal()
        {
            ClearChildren();
            BuildGradingLutTemplate();
            CreateGradingAndAcesPasses();
        }

        AZStd::shared_ptr<RPI::PassTemplate> CreatePassTemplateHelper(
            const Name& templateName, const Name& passClass, bool renderToOwnedImage, const Name& ownedImageName, const char* shaderFilePath)
        {
            auto passTemplate = AZStd::make_shared<RPI::PassTemplate>();
            passTemplate->m_name = templateName;
            passTemplate->m_passClass = passClass;

            // Slots
            passTemplate->m_slots.resize(2);
            RPI::PassSlot& inSlot = passTemplate->m_slots[0];
            inSlot.m_name = "Input";
            inSlot.m_slotType = RPI::PassSlotType::Input;
            inSlot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::SubpassInput;
            inSlot.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;

            RPI::PassSlot& outSlot = passTemplate->m_slots[1];
            outSlot.m_name = "Output";
            outSlot.m_slotType = RPI::PassSlotType::Output;
            outSlot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::RenderTarget;

            passTemplate->m_connections.resize(1);
            RPI::PassConnection& outConnection = passTemplate->m_connections[0];
            if (renderToOwnedImage)
            {
                // If rendering to it's own image attachment
                passTemplate->m_imageAttachments.resize(1);
                RPI::PassImageAttachmentDesc& imageAttachment = passTemplate->m_imageAttachments[0];
                imageAttachment.m_name = ownedImageName;
                imageAttachment.m_sizeSource.m_source.m_pass = "This";
                imageAttachment.m_sizeSource.m_source.m_attachment = "Input";
                imageAttachment.m_formatSource.m_pass = "This";
                imageAttachment.m_formatSource.m_attachment = "Input";
                imageAttachment.m_imageDescriptor.m_bindFlags = RHI::ImageBindFlags::CopyRead | RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite;

                outConnection.m_localSlot = "Output";
                outConnection.m_attachmentRef.m_pass = "This";
                outConnection.m_attachmentRef.m_attachment = ownedImageName;
            }
            else
            {
                // Output connection will be the parent's output
                outConnection.m_localSlot = "Output";
                outConnection.m_attachmentRef.m_pass = "Parent";
                outConnection.m_attachmentRef.m_attachment = "Output";
            }

            AZStd::string azshaderPath = shaderFilePath;
            AZ::StringFunc::Path::ReplaceExtension(azshaderPath, "azshader");

            Data::AssetId shaderAssetId;
            Data::AssetCatalogRequestBus::BroadcastResult(
                shaderAssetId, &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                azshaderPath.c_str(), azrtti_typeid<RPI::ShaderAsset>(), false);
            if (!shaderAssetId.IsValid())
            {
                AZ_Assert(false, "[DisplayMapperPass] Unable to obtain asset id for %s.", shaderFilePath);
                return nullptr;
            }

            AZStd::shared_ptr<RPI::FullscreenTrianglePassData> passData = AZStd::make_shared<RPI::FullscreenTrianglePassData>();
            passData->m_shaderAsset.m_filePath = shaderFilePath;
            passData->m_shaderAsset.m_assetId = shaderAssetId;
            passTemplate->m_passData = AZStd::move(passData);

            return passTemplate;
        }

        AZStd::shared_ptr<RPI::PassTemplate> CreateBakeAcesLutPassTemplateHelper(
            const Name& templateName, const Name& passClass, const char* shaderFilePath)
        {
            auto passTemplate = AZStd::make_shared<RPI::PassTemplate>();
            passTemplate->m_name = templateName;
            passTemplate->m_passClass = passClass;

            // Slots
            passTemplate->m_slots.resize(1);
            RPI::PassSlot& outSlot = passTemplate->m_slots[0];
            outSlot.m_name = "Output";
            outSlot.m_slotType = RPI::PassSlotType::Output;
            outSlot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
            outSlot.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Clear;

            Data::AssetId shaderAssetId;
            Data::AssetCatalogRequestBus::BroadcastResult(
                shaderAssetId, &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                shaderFilePath, azrtti_typeid<RPI::ShaderAsset>(), false);
            if (!shaderAssetId.IsValid())
            {
                AZ_Assert(false, "[DisplayMapperPass] Unable to obtain asset id for %s.", shaderFilePath);
                return nullptr;
            }

            AZStd::shared_ptr<RPI::ComputePassData> passData = AZStd::make_shared<RPI::ComputePassData>();
            passData->m_totalNumberOfThreadsX = 32;
            passData->m_totalNumberOfThreadsY = 32;
            passData->m_totalNumberOfThreadsZ = 32;
            passData->m_shaderReference.m_filePath = shaderFilePath;
            passData->m_shaderReference.m_assetId = shaderAssetId;
            passTemplate->m_passData = AZStd::move(passData);

            return passTemplate;
        }

        /**
         * Create a pass templates for HDR and LDR grading passes, and the output transform passes.
         */
        void DisplayMapperPass::BuildGradingLutTemplate()
        {
            // Pass template names
            const Name acesOutputTransformTemplateName = Name{ "AcesOutputTransformTemplate" };
            const Name acesOutputTransformLutTemplateName = Name{ "AcesOutputTransformLutTemplate" };
            const Name bakeAcesOutputTransformLutTemplateName = Name{ "BakeAcesOutputTransformLutTemplate" };
            const Name displayMapperPassthroughTemplateName = Name{ "DisplayMapperPassthroughTemplate" };
            const Name displayMapperSRGBTemplateName = Name{ "DisplayMapperSRGBTemplate" };
            const Name hdrGradingLutTemplateName = Name{ "HdrGradingLutTemplate" };
            const Name ldrGradingLutTemplateName = Name{ "LdrGradingLutTemplate" };
            const Name outputTransformTemplateName = Name{ "OutputTransformTemplate" };

            // Pass classes 
            const Name acesOutputTransformPassClassName = Name{ "AcesOutputTransformPass" };
            const Name acesOutputTransformLutPassClassName = Name{ "AcesOutputTransformLutPass" };
            const Name bakeAcesOutputTransformLutPassClassName = Name{ "BakeAcesOutputTransformLutPass" };
            const Name displayMapperFullScreenPassClassName = Name{ "DisplayMapperFullScreenPass" };
            const Name applyShaperLookupTablePassClassName = Name{ "ApplyShaperLookupTablePass" };
            const Name outputTransformPassClassName = Name{ "OutputTransformPass" };

            // Names of owned image attachments that may be used
            const Name hdrGradingImageName = Name{ "HdrGradingImage" };
            const Name outputTransformImageName = Name{ "OutputTransformImage" };

            const char* acesOuputTransformShaderPath = "Shaders/PostProcessing/DisplayMapper.azshader";
            const char* acesOuputTransformLutShaderPath = "Shaders/PostProcessing/AcesOutputTransformLut.azshader";
            const char* bakeAcesOuputTransformLutShaderPath = "Shaders/PostProcessing/BakeAcesOutputTransformLutCS.azshader";
            const char* passthroughShaderPath = "Shaders/PostProcessing/FullscreenCopy.azshader";
            const char* sRGBShaderPath = "Shaders/PostProcessing/DisplayMapperSRGB.azshader";
            const char* applyShaperLookupTableShaderFilePath = "Shaders/PostProcessing/ApplyShaperLookupTable.azshader";
            const char* outputTransformShaderPath = m_outputTransformOverrideShader.m_assetId.IsValid()
                ? m_outputTransformOverrideShader.m_filePath.c_str()
                : "Shaders/PostProcessing/OutputTransform.azshader";

            // Output transform templates. If there is no LDR grading LUT pass, then the output transform pass is the final pass
            // and will render to the DisplayMapper's output attachment. Otherwise, it renders to its own attachment image.
            bool useSeparateLdrGradingLutPass = UsesLdrGradingLutPass();
            // ACES
            m_acesOutputTransformTemplate.reset();
            m_acesOutputTransformTemplate = CreatePassTemplateHelper(
                acesOutputTransformTemplateName,
                acesOutputTransformPassClassName,
                useSeparateLdrGradingLutPass,
                outputTransformImageName,
                acesOuputTransformShaderPath);
            // ACES LUT
            m_acesOutputTransformLutTemplate.reset();
            m_acesOutputTransformLutTemplate = CreatePassTemplateHelper(
                acesOutputTransformLutTemplateName,
                acesOutputTransformLutPassClassName,
                useSeparateLdrGradingLutPass,
                outputTransformImageName,
                acesOuputTransformLutShaderPath);
            // Bake ACES LUT
            m_bakeAcesOutputTransformLutTemplate.reset();
            m_bakeAcesOutputTransformLutTemplate = CreateBakeAcesLutPassTemplateHelper(
                bakeAcesOutputTransformLutTemplateName,
                bakeAcesOutputTransformLutPassClassName,
                bakeAcesOuputTransformLutShaderPath);
            // Passthrough
            m_passthroughTemplate.reset();
            m_passthroughTemplate = CreatePassTemplateHelper(
                displayMapperPassthroughTemplateName,
                displayMapperFullScreenPassClassName,
                useSeparateLdrGradingLutPass,
                outputTransformImageName,
                passthroughShaderPath);
            // sRGB
            m_sRGBTemplate.reset();
            m_sRGBTemplate = CreatePassTemplateHelper(
                displayMapperSRGBTemplateName,
                displayMapperFullScreenPassClassName,
                useSeparateLdrGradingLutPass,
                outputTransformImageName,
                sRGBShaderPath);
            // Output Transform
            m_outputTransformTemplate.reset();
            m_outputTransformTemplate = CreatePassTemplateHelper(
                outputTransformTemplateName,
                outputTransformPassClassName,
                useSeparateLdrGradingLutPass,
                outputTransformImageName,
                outputTransformShaderPath);

            // LDR grading LUT pass, if enabled, is the final pass and so it will render into the DisplayMapper's output attachment.
            m_ldrGradingLookupTableTemplate.reset();
            m_ldrGradingLookupTableTemplate = CreatePassTemplateHelper(
                ldrGradingLutTemplateName, applyShaperLookupTablePassClassName,
                false, Name{ "" }, applyShaperLookupTableShaderFilePath);
        }

        template<typename PassType>
            RPI::Ptr<PassType> CreatePassHelper(RPI::PassSystemInterface* passSystem, AZStd::shared_ptr<RPI::PassTemplate> passTemplate, const Name& passName)
        {
            RPI::Ptr<RPI::Pass> pass = passSystem->CreatePassFromTemplate(passTemplate, passName);
            if (!pass)
            {
                AZ_Assert(false, "[DisplayMapperPass] Unable to create %s.", passName.GetCStr());
                return nullptr;
            }

            RPI::Ptr<PassType> returnPass = static_cast<PassType*>(pass.get());
            return AZStd::move(returnPass);
        }

        void DisplayMapperPass::CreateGradingAndAcesPasses()
        {
            RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();

            if (m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::Aces)
            {
                // ACES path
                m_acesOutputTransformPass = CreatePassHelper<AcesOutputTransformPass>(passSystem, m_acesOutputTransformTemplate, m_acesOutputTransformPassName);
            }
            else if (m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::AcesLut)
            {
                // Aces LUT path
                m_bakeAcesOutputTransformLutPass = CreatePassHelper<BakeAcesOutputTransformLutPass>(passSystem, m_bakeAcesOutputTransformLutTemplate, m_bakeAcesOutputTransformLutPassName);
                m_acesOutputTransformLutPass = CreatePassHelper<AcesOutputTransformLutPass>(passSystem, m_acesOutputTransformLutTemplate, m_acesOutputTransformLutPassName);
            }
            else if (m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::Passthrough)
            {
                // Passthrough
                // [GFX TODO][ATOM-4189] Optimize the passthrough function in the DisplayMapper
                // Only need to create this if LDR grading LUT pass is not used.
                if (!UsesLdrGradingLutPass())
                {
                    m_displayMapperPassthroughPass = CreatePassHelper<DisplayMapperFullScreenPass>(passSystem, m_passthroughTemplate, m_displayMapperPassthroughPassName);
                }
            }
            else if (m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::GammaSRGB)
            {
                m_displayMapperSRGBPass = CreatePassHelper<DisplayMapperFullScreenPass>(passSystem, m_sRGBTemplate, m_displayMapperSRGBPassName);
            }
            else if (UsesOutputTransformPass())
            {
                m_outputTransformPass = CreatePassHelper<OutputTransformPass>(passSystem, m_outputTransformTemplate, m_outputTransformPassName);
                ToneMapperType type = ToneMapperType::None;
                switch (m_displayMapperConfigurationDescriptor.m_operationType)
                {
                case DisplayMapperOperationType::Reinhard:
                    type = ToneMapperType::Reinhard;
                    break;
                case DisplayMapperOperationType::AcesFitted:
                    type = ToneMapperType::AcesFitted;
                    break;
                case DisplayMapperOperationType::AcesFilmic:
                    type = ToneMapperType::AcesFilmic;
                    break;
                case DisplayMapperOperationType::Filmic:
                    type = ToneMapperType::Filmic;
                    break;
                default:
                    AZ_Assert(false, "Invalid tonemapper type %d", m_displayMapperConfigurationDescriptor.m_operationType);
                    break;
                }
                m_outputTransformPass->SetToneMapperType(type);
                if (m_displayMapperConfigurationDescriptor.m_ldrGradingLutEnabled &&
                    m_mergeLdrGradingLut &&
                    m_displayMapperConfigurationDescriptor.m_ldrColorGradingLut.GetId().IsValid())
                {
                    m_outputTransformPass->SetLutAssetId(m_displayMapperConfigurationDescriptor.m_ldrColorGradingLut.GetId());
                }
            }

            if (UsesLdrGradingLutPass())
            {
                // ID should be valid, maybe no need to check again here.
                if (m_displayMapperConfigurationDescriptor.m_ldrColorGradingLut.GetId().IsValid())
                {
                    // For LDR tonemapping, no need to set the shaper function as the default identity shaper is assumed.
                    m_ldrGradingLookupTablePass = CreatePassHelper<ApplyShaperLookupTablePass>(passSystem, m_ldrGradingLookupTableTemplate, m_ldrGradingLookupTablePassName);
                    m_ldrGradingLookupTablePass->SetLutAssetId(m_displayMapperConfigurationDescriptor.m_ldrColorGradingLut.GetId());
                }
            }

            // Add the children as necessary
            if (m_acesOutputTransformPass)
            {
                AddChild(m_acesOutputTransformPass);
            }
            if (m_bakeAcesOutputTransformLutPass)
            {
                AddChild(m_bakeAcesOutputTransformLutPass);
            }
            if (m_acesOutputTransformLutPass)
            {
                AddChild(m_acesOutputTransformLutPass);
            }
            if (m_displayMapperPassthroughPass)
            {
                AddChild(m_displayMapperPassthroughPass);
            }
            if (m_displayMapperSRGBPass)
            {
                AddChild(m_displayMapperSRGBPass);
            }
            if (m_outputTransformPass)
            {
                AddChild(m_outputTransformPass);
            }
            if (m_ldrGradingLookupTablePass)
            {
                AddChild(m_ldrGradingLookupTablePass);
            }
        }

        void DisplayMapperPass::UpdateDisplayMapperConfiguration()
        {
            // If no configuration is found, then use the default value that was set for the pass.
            DisplayMapperConfigurationDescriptor desc = m_defaultDisplayMapperConfigurationDescriptor;
            AZ::RPI::Scene* scene = GetScene();
            if (scene)
            {
                AcesDisplayMapperFeatureProcessor* fp = scene->GetFeatureProcessor<AcesDisplayMapperFeatureProcessor>();
                if (fp)
                {
                    auto fpDesc = fp->GetDisplayMapperConfiguration();
                    if (fpDesc)
                    {
                        desc = *fpDesc;
                        // Check to ensure that a valid LUT has been set
                        if (desc.m_ldrGradingLutEnabled)
                        {
                            if (!desc.m_ldrColorGradingLut.GetId().IsValid())
                            {
                                desc.m_ldrGradingLutEnabled = false;
                            }
                        }
                    }
                }
            }

            if (desc.m_operationType != m_displayMapperConfigurationDescriptor.m_operationType ||
                desc.m_ldrGradingLutEnabled != m_displayMapperConfigurationDescriptor.m_ldrGradingLutEnabled ||
                desc.m_ldrColorGradingLut != m_displayMapperConfigurationDescriptor.m_ldrColorGradingLut ||
                desc.m_acesParameterOverrides.m_overrideDefaults !=
                    m_displayMapperConfigurationDescriptor.m_acesParameterOverrides.m_overrideDefaults)
            {
                m_flags.m_createChildren = true;
                QueueForBuildAndInitialization();
            }
            m_displayMapperConfigurationDescriptor = desc;
        }

        void DisplayMapperPass::ClearChildren()
        {
            RemoveChildren();

            m_acesOutputTransformPass = nullptr;
            m_bakeAcesOutputTransformLutPass = nullptr;
            m_acesOutputTransformLutPass = nullptr;
            m_displayMapperPassthroughPass = nullptr;
            m_displayMapperSRGBPass = nullptr;
            m_ldrGradingLookupTablePass = nullptr;
            m_outputTransformPass = nullptr;
        }

        bool DisplayMapperPass::UsesLdrGradingLutPass() const
        {
            // Merging the ldr color grading is only supported for the OutputTransform pass at the moment.
            return m_displayMapperConfigurationDescriptor.m_ldrGradingLutEnabled && !(m_mergeLdrGradingLut && UsesOutputTransformPass());
        }

        bool DisplayMapperPass::UsesOutputTransformPass() const
        {
            return m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::Reinhard ||
                m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::AcesFitted ||
                m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::Filmic ||
                m_displayMapperConfigurationDescriptor.m_operationType == DisplayMapperOperationType::AcesFilmic;
        }
    }   // namespace Render
}   // namespace AZ
