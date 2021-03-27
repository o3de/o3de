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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzFramework/Windowing/WindowBus.h>

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>

namespace AZ
{
    namespace Render
    {
        AZ_ENUM_CLASS(FrameCaptureResult,
            None,
            Success,
            FileWriteError,
            InvalidArgument,
            UnsupportedFormat,
            InternalError
        )

        class FrameCaptureRequests
            : public EBusTraits
        {
        public:
            virtual ~FrameCaptureRequests() = default;
            
            //! Capture final screen output for the specified window and save it to given file path.
            //! The image format is determinate by file extension
            //! Current supported formats include ppm and dds. 
            virtual bool CaptureScreenshotForWindow(const AZStd::string& filePath, AzFramework::NativeWindowHandle windowHandle) = 0;

            //! Similar to CaptureScreenshotForWindow except it's capturing the screen shot for default window 
            virtual bool CaptureScreenshot(const AZStd::string& filePath) = 0;

            //! Capture a screenshot and save it to a file if the pass image attachment preview is enabled.
            //! It will return false if the preview is not enabled.
            virtual bool CaptureScreenshotWithPreview(const AZStd::string& outputFilePath) = 0;
            
            //! Save a buffer attachment or a image attachment binded to a pass's slot to a data file.
            //! @param passHierarchy For finding the pass by using PassHierarchyFilter
            //! @param slotName Name of the pass's slot. The attachment bound to this slot will be captured.
            //! @param outputFilename The output file path. 
            virtual bool CapturePassAttachment(const AZStd::vector<AZStd::string>& passHierarchy, const AZStd::string& slotName
                , const AZStd::string& outputFilePath) = 0;

            //! Similar to CapturePassAttachment. But instead of saving the read back result to a file, it will call the callback function provide
            //! in the input when callback is finished
            virtual bool CapturePassAttachmentWithCallback(const AZStd::vector<AZStd::string>& passHierarchy, const AZStd::string& slotName
                , RPI::AttachmentReadback::CallbackFunction callback) = 0;

        };
        using FrameCaptureRequestBus = EBus<FrameCaptureRequests>;

        class FrameCaptureNotifications
            : public EBusTraits
        {
        public:
            virtual ~FrameCaptureNotifications() = default;
            
            //! Notify when the current capture is finished
            //! @param result result code
            //! @param info The output file path or error information which depends on the result. 
            virtual void OnCaptureFinished(FrameCaptureResult result, const AZStd::string& info) = 0;
        };
        using FrameCaptureNotificationBus = EBus<FrameCaptureNotifications>;

        //! Stores the result of a frame capture request.
        //! Includes the result type along with an optional error message if the request did not complete successfully.
        struct FrameCaptureOutputResult
        {
            FrameCaptureResult m_result; //!< Outcome after attempting to capture a frame.
            AZStd::optional<AZStd::string> m_errorMessage; //!< If the capture did not succeed, an optional diagnostic message is set.
        };

        //! Writes out content of ReadbackResult in the Dds image format.
        FrameCaptureOutputResult DdsFrameCaptureOutput(
            const AZStd::string& outputFilePath, const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult);

    } // namespace Render

    AZ_TYPE_INFO_SPECIALIZE(Render::FrameCaptureResult, "{F0B013CE-DFAE-4743-B123-EB1EE1705E03}");
} // namespace AZ
