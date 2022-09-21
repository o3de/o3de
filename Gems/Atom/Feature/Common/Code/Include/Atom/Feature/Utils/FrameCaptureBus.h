/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzFramework/Windowing/WindowBus.h>

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/Utils/ImageComparison.h>

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

        using FrameCaptureId = uint32_t;
        constexpr FrameCaptureId InvalidFrameCaptureId = aznumeric_cast<FrameCaptureId>(-1);
        class FrameCaptureRequests
            : public EBusTraits
        {
        public:
            virtual ~FrameCaptureRequests() {};

            //! Return true if frame capture is available.
            //! It may return false if null renderer is used.
            //! If the frame capture is not available, all capture functions in this interface will return InvalidFrameCaptureId
            virtual bool CanCapture() const = 0;
            
            //! Capture final screen output for the specified window and save it to given file path.
            //! The image format is determinate by file extension
            //! Current supported formats include ppm and dds. 
            //! @param filepath The output file path. 
            //! @param windowHandle The handle to the AzFrameWork::NativeWindow that is being captured
            //! @return value is the frame capture Id, on failure it will return InvalidFrameCaptureId
            virtual FrameCaptureId CaptureScreenshotForWindow(const AZStd::string& filePath, AzFramework::NativeWindowHandle windowHandle) = 0;

            //! Similar to CaptureScreenshotForWindow except it's capturing the screen shot for default window 
            //! @param filepath The output file path. 
            //! @return value is the frame capture Id, on failure it will return InvalidFrameCaptureId
            virtual FrameCaptureId CaptureScreenshot(const AZStd::string& filePath) = 0;

            //! Capture a screenshot and save it to a file if the pass image attachment preview is enabled.
            //! It will return InvalidFrameCaptureId if the preview is not enabled.
            //! @param outputFilePath The output file path. 
            //! @return value is the frame capture Id, on failure it will return InvalidFrameCaptureId
            virtual FrameCaptureId CaptureScreenshotWithPreview(const AZStd::string& outputFilePath) = 0;
            
            //! Save a buffer attachment or a image attachment binded to a pass's slot to a data file.
            //! @param passHierarchy For finding the pass by using a pass hierarchy filter. Check PassFilter::CreateWithPassHierarchy() function for detail
            //! @param slotName Name of the pass's slot. The attachment bound to this slot will be captured.
            //! @param option Only valid for an InputOutput attachment. Use PassAttachmentReadbackOption::Input to capture the input state
            //!               and use PassAttachmentReadbackOption::Output to capture the output state
            //! @param outputFilePath The output file path. 
            //! @return value is the frame capture Id, on failure it will return InvalidFrameCaptureId
            virtual FrameCaptureId CapturePassAttachment(const AZStd::vector<AZStd::string>& passHierarchy, const AZStd::string& slotName
                , const AZStd::string& outputFilePath, RPI::PassAttachmentReadbackOption option) = 0;

            //! Similar to CapturePassAttachment. But instead of saving the read back result to a file, it will call the callback function provide
            //! in the input when callback is finished
            //! @param passHierarchy For finding the pass by using a pass hierarchy filter. Check PassFilter::CreateWithPassHierarchy() function for detail
            //! @param slotName Name of the pass's slot. The attachment bound to this slot will be captured.
            //! @param callback function to call when the data is ready to read
            //! @param option Only valid for an InputOutput attachment. Use PassAttachmentReadbackOption::Input to capture the input state
            //!               and use PassAttachmentReadbackOption::Output to capture the output state
            //! @return value is the frame capture Id, on failure it will return InvalidFrameCaptureId
            virtual FrameCaptureId CapturePassAttachmentWithCallback(const AZStd::vector<AZStd::string>& passHierarchy, const AZStd::string& slotName
                , RPI::AttachmentReadback::CallbackFunction callback, RPI::PassAttachmentReadbackOption option) = 0;
        };
        using FrameCaptureRequestBus = EBus<FrameCaptureRequests>;

        class FrameCaptureNotifications
            : public EBusTraits
        {
        public:
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = FrameCaptureId;

            virtual ~FrameCaptureNotifications() {};
            
            //! Notify when a capture is finished, you may receive notifications for other captures than your own
            //! @param frameCaptureId The frame capture id returned when the capture was triggered. This is returned here in case client code has triggered multiple captures
            //! @param result result code
            //! @param info The output file path or error information which depends on the result. 
            virtual void OnFrameCaptureFinished(FrameCaptureResult result, const AZStd::string& info) = 0;
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

        //! Writes out content of ReadbackResult in the Ppm image format.
        FrameCaptureOutputResult PpmFrameCaptureOutput(
            const AZStd::string& outputFilePath, const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult);

    } // namespace Render

    AZ_TYPE_INFO_SPECIALIZE(Render::FrameCaptureResult, "{F0B013CE-DFAE-4743-B123-EB1EE1705E03}");
} // namespace AZ
