/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <Atom/Feature/Utils/FrameCaptureBus.h>

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>

namespace AZ
{
    namespace Render
    {
        //! System component to handle FrameCaptureRequestBus.
        class FrameCaptureSystemComponent final
            : public AZ::Component
            , public FrameCaptureRequestBus::Handler
            , public AZ::SystemTickBus::Handler
        {
        public:
            AZ_COMPONENT(FrameCaptureSystemComponent, "{53931220-19E7-4DE4-AF29-C4BB16E251D1}");

            static void Reflect(AZ::ReflectContext* context);

            void Activate() override;
            void Deactivate() override;

            // FrameCaptureRequestBus overrides ...
            bool CanCapture() const override;
            bool CaptureScreenshot(const AZStd::string& filePath) override;
            bool CaptureScreenshotForWindow(const AZStd::string& filePath, AzFramework::NativeWindowHandle windowHandle) override;
            bool CaptureScreenshotWithPreview(const AZStd::string& outputFilePath) override;
            bool CapturePassAttachment(const AZStd::vector<AZStd::string>& passHierarchy, const AZStd::string& slotName, const AZStd::string& outputFilePath,
                RPI::PassAttachmentReadbackOption option) override;
            bool CapturePassAttachmentWithCallback(const AZStd::vector<AZStd::string>& passHierarchy, const AZStd::string& slotName
                , RPI::AttachmentReadback::CallbackFunction callback, RPI::PassAttachmentReadbackOption option) override;

        private:
            void CaptureAttachmentCallback(const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult);

            void InitReadback();

            // SystemTickBus overrides ...
            void OnSystemTick() override;

            AZStd::string ResolvePath(const AZStd::string& filePath);

            AZStd::shared_ptr<AZ::RPI::AttachmentReadback> m_readback;
            AZStd::string m_outputFilePath;

            enum State
            {
                Idle,
                Pending,
                WasSuccess,
                WasFailure
            };

            State m_state = State::Idle;
            FrameCaptureResult m_result = FrameCaptureResult::None;
            AZStd::string m_latestCaptureInfo;
            RPI::AttachmentReadback::CallbackFunction m_customCallback;
        };
    }
}
