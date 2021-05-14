/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>


namespace AZ::Render
{
    class TaaPass : public RPI::ComputePass
    {
        using Base = RPI::ComputePass;
        AZ_RPI_PASS(TaaPass);
        
    public:
        AZ_RTTI(AZ::Render::TaaPass, "{AB3BD4EA-33D7-477F-82B4-21DDFB517499}", Base);
        AZ_CLASS_ALLOCATOR(TaaPass, SystemAllocator, 0);
        virtual ~TaaPass() = default;
        
        /// Creates a TaaPass
        static RPI::Ptr<TaaPass> Create(const RPI::PassDescriptor& descriptor);
        
        static Name GetTaaPassTemplateName()
        {
            return Name("TaaTemplate");
        }

    private:

        TaaPass(const RPI::PassDescriptor& descriptor);
        
        // Scope producer functions...
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;
        
        // Pass behavior overrides...
        void FrameBeginInternal(FramePrepareParams params) override;
        void ResetInternal()override;
        void BuildAttachmentsInternal() override;

        void UpdateAttachmentImage(RPI::Ptr<RPI::PassAttachment>& attachment);

        RHI::ShaderInputNameIndex m_outputIndex = "m_output";
        RHI::ShaderInputNameIndex m_lastFrameAccumulationIndex = "m_lastFrameAccumulation";
        RHI::ShaderInputNameIndex m_constantDataIndex = "m_constantData";

        Data::Instance<RPI::PassAttachment> m_accumulationAttachments[2];

        RPI::PassAttachmentBinding* m_inputColorBinding = nullptr;
        RPI::PassAttachmentBinding* m_lastFrameAccumulationBinding = nullptr;
        RPI::PassAttachmentBinding* m_outputColorBinding = nullptr;

        uint8_t m_accumulationOuptutIndex = 0;

    };
} // namespace AZ::Render
