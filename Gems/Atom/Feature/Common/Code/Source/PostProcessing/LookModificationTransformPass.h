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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace AZ
{
    namespace Render
    {
        /**
         *  The look modification transform pass.
         */
        class LookModificationPass
            : public RPI::ParentPass
        {
        public:
            AZ_RTTI(LookModificationPass, "{68C3A664-FB97-40ED-9638-21938D6692B3}", RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(LookModificationPass, SystemAllocator, 0);
            virtual ~LookModificationPass() = default;

            //! Creates a LookModificationPass
            static RPI::Ptr<LookModificationPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            LookModificationPass(const RPI::PassDescriptor& descriptor);

            //! Pass overrides ...
            void FrameBeginInternal(FramePrepareParams params) override;

            void BuildAttachmentsInternal() override;

        private:
            const RPI::PassAttachmentBinding* m_swapChainAttachmentBinding = nullptr;
            RHI::Format m_displayBufferFormat = RHI::Format::Unknown;
            OutputDeviceTransformType m_outputDeviceTransformType = OutputDeviceTransformType_48Nits;
            ShaperParams m_shaperParams;
        };
    }   // namespace Render
}   // namespace AZ
