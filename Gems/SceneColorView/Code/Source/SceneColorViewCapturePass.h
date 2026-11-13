/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>

namespace SceneColorView
{
    class SceneColorViewCapturePass final : public AZ::RPI::FullscreenTrianglePass
    {
    public:
        AZ_RTTI(SceneColorViewCapturePass, "{867C7D6F-9E32-4718-B111-0F868C687648}", AZ::RPI::FullscreenTrianglePass);
        AZ_CLASS_ALLOCATOR(SceneColorViewCapturePass, AZ::SystemAllocator);
        static AZ::RPI::Ptr<SceneColorViewCapturePass> Create(const AZ::RPI::PassDescriptor& descriptor);
        void SetConsumer(AZ::RPI::Ptr<AZ::RPI::Pass> consumer) { m_consumer = consumer; }

    private:
        explicit SceneColorViewCapturePass(const AZ::RPI::PassDescriptor& descriptor);
        void BuildInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;
        bool EnsureImage(uint32_t width, uint32_t height);
        void SetConsumerAttachment(AZ::RPI::Ptr<AZ::RPI::PassAttachment> attachment);

        AZ::RPI::Ptr<AZ::RPI::Pass> m_consumer;
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_image;
        AZ::RPI::Ptr<AZ::RPI::PassAttachment> m_captureAttachment;
        bool m_reportedUnsupported = false;
    };
}
