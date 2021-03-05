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

namespace AZ
{
    namespace Render
    {
        //! CheckerboardColorResolvePass is to resolve checkerboard color render targets
        //! by using frame n and frame n-1 2xMS color render targets and corresponding depth buffers
        class CheckerboardColorResolvePass final
            : public RPI::ComputePass
        {
            using Base = RPI::ComputePass;
            AZ_RPI_PASS(CheckerboardColorResolvePass);
      
        public:
            AZ_RTTI(CheckerboardColorResolvePass, "{62CA67F2-7957-4951-926B-BACD7069A399}", Base);
            AZ_CLASS_ALLOCATOR(CheckerboardColorResolvePass, SystemAllocator, 0);

            ~CheckerboardColorResolvePass() = default;
            static RPI::Ptr<CheckerboardColorResolvePass> Create(const RPI::PassDescriptor& descriptor);

            enum class DebugRenderType : uint32_t
            {
                None = 0,
                MotionVectors = AZ_BIT(0), //0x1
                MissingPixels = AZ_BIT(1),
                QuadrantMotionPixels = AZ_BIT(2),
                OddFrame = AZ_BIT(3),
                OddEven = AZ_BIT(4),
                ObstructedPixels = AZ_BIT(5),
            };

            void SetDebugRender(DebugRenderType flag);
            DebugRenderType GetDebugRenderType() const;

            void SetEnableCheckOcclusion(bool enabled);
            bool IsCheckingOcclusion() const;

        protected:
            // Pass overrides...
            void FrameBeginInternal(FramePrepareParams params) override;
            void BuildAttachmentsInternal() override;
            void FrameEndInternal() override;

            void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;

            // one time initialization
            void Init();

        private:
            CheckerboardColorResolvePass() = delete;
            explicit CheckerboardColorResolvePass(const RPI::PassDescriptor& descriptor);

            AZ::RHI::ShaderInputConstantIndex m_constantsIndex;
                        
            bool m_initialized = false;
            Matrix4x4 m_prevClipToWorld;
            uint8_t m_frameOffset = 0;
            DebugRenderType m_debugRenderType = DebugRenderType::None;
            bool m_checkOcclusion = false;
        };
    } // namespace Render
} // namespace AZ
