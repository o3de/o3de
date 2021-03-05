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
            , public AZ::TickBus::Handler
        {
        public:
            AZ_COMPONENT(FrameCaptureSystemComponent, "{53931220-19E7-4DE4-AF29-C4BB16E251D1}");

            static void Reflect(AZ::ReflectContext* context);

            void Activate() override;
            void Deactivate() override;

            // FrameCaptureRequestBus overrides ...
            bool CaptureScreenshot(const AZStd::string& filePath) override;
            bool CaptureScreenshotForWindow(const AZStd::string& filePath, AzFramework::NativeWindowHandle windowHandle) override;
            bool CaptureScreenshotWithPreview(const AZStd::string& outputFilePath) override;
            bool CapturePassAttachment(const AZStd::vector<AZStd::string>& passHierarchy, const AZStd::string& slotName, const AZStd::string& outputFilePath) override;
            bool CapturePassAttachmentWithCallback(const AZStd::vector<AZStd::string>& passHierarchy, const AZStd::string& slotName
                , RPI::AttachmentReadback::CallbackFunction callback) override;

        private:
            void CaptureAttachmentCallback(const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult);

            void InitReadback();

            void OnTick(float deltaTime, ScriptTimePoint time) override;

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
