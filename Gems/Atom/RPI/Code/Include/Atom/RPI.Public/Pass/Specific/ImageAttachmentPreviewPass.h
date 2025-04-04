/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomCore/Instance/Instance.h>

#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RPI
    {
        class RenderPass;

        //! A scope producer to copy the input attachment and output a copy of this attachment
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API ImageAttachmentCopy final
            : public RHI::ScopeProducer
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ImageAttachmentPreviewPass;
        public:
            AZ_RTTI(ImageAttachmentCopy, "{27E35230-48D1-4950-8489-F301A45D4A0B}", RHI::ScopeProducer);
            AZ_CLASS_ALLOCATOR(ImageAttachmentCopy, SystemAllocator);

            ImageAttachmentCopy() = default;
            ~ImageAttachmentCopy() = default;

            void SetImageAttachment(RHI::AttachmentId srcAttachmentId, RHI::AttachmentId destAttachmentId);

            void FrameBegin(Pass::FramePrepareParams params);

            void Reset();

            void InvalidateDestImage();

        protected:
            // RHI::ScopeProducer overrides...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override;

        private:
            RHI::AttachmentId m_srcAttachmentId;
            RHI::AttachmentId m_destAttachmentId;

            Data::Instance<AttachmentImage> m_destImage;
            u16 m_sourceArraySlice = 0;

            // Copy item to be submitted to command list
            RHI::CopyItem m_copyItem;
        };

        //! Render preview of specified image attachment to the selected output attachment.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API ImageAttachmentPreviewPass final
            : public Pass
            , public RHI::ScopeProducer
            , public Data::AssetBus::Handler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            AZ_RPI_PASS(ImageAttachmentPreviewPass);

        public:
            AZ_RTTI(ImageAttachmentPreviewPass, "{E6076B8E-E840-4C22-89A8-32C73FEEEBF9}", Pass);
            AZ_CLASS_ALLOCATOR(ImageAttachmentPreviewPass, SystemAllocator);

            //! Creates an ImageAttachmentPreviewPass
            static Ptr<ImageAttachmentPreviewPass> Create(const PassDescriptor& descriptor);

            ~ImageAttachmentPreviewPass();
            
            //! Preview the PassAttachment of a pass' PassAttachmentBinding
            void PreviewImageAttachmentForPass(Pass* pass, const PassAttachment* passAttachment, RenderPipeline* previewOutputPipeline = nullptr, u32 imageArraySlice = 0);

            //! Set the output color attachment for this pass
            void SetOutputColorAttachment(RHI::Ptr<PassAttachment> outputImageAttachment);

            //! clear the image attachments for preview
            void ClearPreviewAttachment();

            //! Set the preview location on the output attachment.
            //! Assuming the left top corner of output is (0, 0) and right bottom corner is (1, 1)
            void SetPreviewLocation(AZ::Vector2 position, AZ::Vector2 size, bool keepAspectRatio = true);

            //! Readback the output color attachment
            bool ReadbackOutput(AZStd::shared_ptr<AttachmentReadback> readback);

            //! Set a min/max range for remapping the preview output, to increase contrast. The default of 0-1 is a no-op.
            void SetColorTransformRange(float colorTransformRange[2]);

        private:
            explicit ImageAttachmentPreviewPass(const PassDescriptor& descriptor);

            // Data::AssetBus::Handler overrides...
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

            // Creation render related resources
            void LoadShader();

            // Pass overrides
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // RHI::ScopeProducer overrides...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override;

            void ClearDrawData();

            // Image types. This is matching the option defined in ImageAttachmentsPreview.azsl
            enum class ImageType : uint32_t
            {
                Image2d = 0,        // Regular 2d image
                Image2dMs,          // 2d image with multisampler

                ImageTypeCount,
                Unsupported = ImageTypeCount
            };

            // For each type of images, they are using one set of data used for preview
            struct ImageTypePreviewInfo
            {
                RHI::ShaderInputImageIndex m_imageInput;
                // Cached pipeline state descriptor
                RHI::PipelineStateDescriptorForDraw m_pipelineStateDescriptor;
                // The draw item for drawing the image preview for this type of image
                RHI::DrawItem m_item{RHI::MultiDevice::AllDevices};

                // Holds the geometry info for the draw call
                RHI::GeometryView m_geometryView;

                // Key to pass to the SRG when desired shader variant isn't found
                ShaderVariantKey m_shaderVariantKeyFallback;

                uint32_t m_imageCount = 0;
            };
            
            // image attachment to be rendered for preview
            RHI::AttachmentId m_imageAttachmentId;

            // render target for the preview
            RHI::Ptr<PassAttachment> m_outputColorAttachment;
            
            RHI::ShaderInputConstantIndex m_colorRangeMinMaxInput;
            float m_attachmentColorTranformRange[2] = {0.0f, 1.0f};

            // shader for render images to the output
            Data::Instance<Shader> m_shader;

            // The shader resource group for this pass
            Data::Instance<ShaderResourceGroup> m_passSrg;
            bool m_passSrgChanged = false;
                        
            AZStd::array<ImageTypePreviewInfo, static_cast<uint32_t>(ImageType::ImageTypeCount)> m_imageTypePreviewInfo;

            // whether to update the draw data for both srg data and draw item
            bool m_updateDrawData = false;
            
            bool m_needsShaderLoad = true;

            RHI::Viewport m_viewport;
            RHI::Scissor m_scissor;

            AZStd::shared_ptr<ImageAttachmentCopy> m_attachmentCopy;

            // preview location info
            // defaults to left bottom corner
            AZ::Vector2 m_position = AZ::Vector2(0, 0.6f);
            AZ::Vector2 m_size = AZ::Vector2(0.4f, 0.4f);
            bool m_keepAspectRatio = true;
        };
    }   // namespace RPI
}   // namespace AZ
