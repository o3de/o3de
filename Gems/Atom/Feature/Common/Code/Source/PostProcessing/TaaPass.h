/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>

namespace AZ::Render
{
    //! Custom data for the Taa Pass.
    struct TaaPassData
        : public RPI::ComputePassData
    {
        AZ_RTTI(TaaPassData, "{BCDF5C7D-7A78-4C69-A460-FA6899C3B960}", ComputePassData);
        AZ_CLASS_ALLOCATOR(TaaPassData, SystemAllocator, 0);

        TaaPassData() = default;
        virtual ~TaaPassData() = default;

        static void Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<TaaPassData, RPI::ComputePassData>()
                    ->Version(1)
                    ->Field("NumJitterPositions", &TaaPassData::m_numJitterPositions)
                    ;
            }
        }

        uint32_t m_numJitterPositions = 8;
    };

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
        
    private:

        TaaPass(const RPI::PassDescriptor& descriptor);
        
        // Scope producer functions...
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        
        // Pass behavior overrides...
        void FrameBeginInternal(FramePrepareParams params) override;
        void ResetInternal() override;
        void BuildInternal() override;

        void UpdateAttachmentImage(RPI::Ptr<RPI::PassAttachment>& attachment);

        void SetupSubPixelOffsets(uint32_t haltonX, uint32_t haltonY, uint32_t length);
        void GenerateFilterWeights(AZ::Vector2 jitterOffset);

        RHI::ShaderInputNameIndex m_outputIndex = "m_output";
        RHI::ShaderInputNameIndex m_lastFrameAccumulationIndex = "m_lastFrameAccumulation";
        RHI::ShaderInputNameIndex m_constantDataIndex = "m_constantData";

        Data::Instance<RPI::PassAttachment> m_accumulationAttachments[2];

        RPI::PassAttachmentBinding* m_inputColorBinding = nullptr;
        RPI::PassAttachmentBinding* m_lastFrameAccumulationBinding = nullptr;
        RPI::PassAttachmentBinding* m_outputColorBinding = nullptr;

        struct Offset
        {
            Offset() = default;

            // Constructor for implicit conversion from array output by HaltonSequence.
            Offset(AZStd::array<float, 2> offsets)
                : m_xOffset(offsets[0])
                , m_yOffset(offsets[1])
            {};

            float m_xOffset = 0.0f;
            float m_yOffset = 0.0f;
        };

        AZStd::array<float, 9> m_filterWeights = { 0.0f };

        AZStd::vector<Offset> m_subPixelOffsets;
        uint32_t m_offsetIndex = 0;

        uint8_t m_accumulationOuptutIndex = 0;

    };
} // namespace AZ::Render
