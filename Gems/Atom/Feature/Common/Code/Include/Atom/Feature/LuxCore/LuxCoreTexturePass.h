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

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Reflect/Image/Image.h>

#include <Atom/Feature/LuxCore/RenderTexturePass.h>

namespace AZ
{
    namespace Render
    {
        class LuxCoreTexturePass final
            : public RPI::ParentPass
        {
        public:
            AZ_RTTI(LuxCoreTexturePass, "{A6CA80C0-63A6-4686-A627-B5D1DA04B627}", ParentPass);
            AZ_CLASS_ALLOCATOR(LuxCoreTexturePass, SystemAllocator, 0);

            static RPI::Ptr<LuxCoreTexturePass> Create(const RPI::PassDescriptor& descriptor);

            LuxCoreTexturePass(const RPI::PassDescriptor& descriptor);
            ~LuxCoreTexturePass();

            void SetSourceTexture(Data::Instance<AZ::RPI::Image> image, RHI::Format format);
            void SetIsNormalTexture(bool isNormal);
            void SetReadbackCallback(RPI::AttachmentReadback::CallbackFunction callbackFunciton);

        protected:
            // Pass behavior overrides
            void CreateChildPassesInternal() final;
            void BuildInternal() final;
            void FrameBeginInternal(FramePrepareParams params) final;

        private:

            RPI::Ptr<RPI::Pass> m_renderTargetPass = nullptr;
            AZStd::shared_ptr<RPI::AttachmentReadback> m_readback = nullptr;
            bool m_attachmentReadbackComplete = false;
        };
    }
}
