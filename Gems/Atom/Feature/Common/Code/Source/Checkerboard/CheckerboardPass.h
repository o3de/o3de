/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>

namespace AZ
{
    namespace Render
    {
        //! Checkerboard pass renders scene to a multi-sample target with checkerboard pattern
        //! The checkerboard will be shifted one pixel between odd and even frames
        //! In this customized pass, it creates two imported attachment images to save last frame's color and depth buffer
        //! And it also shifts the viewport for the checkerboard pattern
        class CheckerboardPass final
            : public RPI::RasterPass
        {
            using Base = RPI::RasterPass;
            AZ_RPI_PASS(CheckerboardPass);
      
        public:
            AZ_RTTI(CheckerboardPass, "{C78A4C90-3915-4D8C-80BE-3698CF72C2C1}", Base);
            AZ_CLASS_ALLOCATOR(CheckerboardPass, SystemAllocator, 0);

            ~CheckerboardPass() = default;
            static RPI::Ptr<CheckerboardPass> Create(const RPI::PassDescriptor& descriptor);

            Data::Instance<RPI::AttachmentImage> GetAttachmentImage(Name attachmentName, uint8_t frameOffset);

        protected:
            // Pass overrides...
            void FrameBeginInternal(FramePrepareParams params) override;
            void BuildInternal() override;
            void FrameEndInternal() override;

        private:
            CheckerboardPass() = delete;
            explicit CheckerboardPass(const RPI::PassDescriptor& descriptor);

            // PassAttachment name to AttachmentImages mapping
            // Each pass output image attachment has two AttachmentImage.
            // One for last frame and one for current frame. They will be used in checkerboard resolve
            AZStd::unordered_map<Name, AZStd::array<Data::Instance<RPI::AttachmentImage>, 2>> m_imageAttachments;
            uint8_t m_frameOffset = 0;
        };
    } // namespace Render
} // namespace AZ
