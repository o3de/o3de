/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <Atom/Utils/DdsFile.h>
#include <Atom/Utils/PpmFile.h>

namespace AZ::Render
{
    FrameCaptureOutputResult DdsFrameCaptureOutput(
        const AZStd::string& outputFilePath, const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
    {
        // write the read back result of the image attachment to a dds file
        const auto outcome = AZ::DdsFile::WriteFile(
            outputFilePath,
            { readbackResult.m_imageDescriptor.m_size, readbackResult.m_imageDescriptor.m_format, readbackResult.m_dataBuffer.get() });

        return outcome.IsSuccess() ? FrameCaptureOutputResult{ FrameCaptureResult::Success, AZStd::nullopt }
                                   : FrameCaptureOutputResult{ FrameCaptureResult::InternalError, outcome.GetError().m_message };
    }

    FrameCaptureOutputResult PpmFrameCaptureOutput(
        const AZStd::string& outputFilePath, const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
    {
        // write the read back result of the image attachment to a buffer
        const AZStd::vector<uint8_t> outBuffer = Utils::PpmFile::CreatePpmFromImageBuffer(
            *readbackResult.m_dataBuffer.get(), readbackResult.m_imageDescriptor.m_size, readbackResult.m_imageDescriptor.m_format);

        // write the buffer to a ppm file
        if (IO::FileIOStream fileStream(outputFilePath.c_str(), IO::OpenMode::ModeWrite | IO::OpenMode::ModeCreatePath);
            fileStream.IsOpen())
        {
            fileStream.Write(outBuffer.size(), outBuffer.data());
            fileStream.Close();

            return FrameCaptureOutputResult{ FrameCaptureResult::Success, AZStd::nullopt };
        }

        return FrameCaptureOutputResult{ FrameCaptureResult::FileWriteError,
                                         AZStd::string::format("Failed to open file %s for writing", outputFilePath.c_str()) };
    }
} // namespace AZ::Render
