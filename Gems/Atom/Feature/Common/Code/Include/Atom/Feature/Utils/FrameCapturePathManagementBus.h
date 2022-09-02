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
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Render
    {
        class FrameCapturePathManagementRequests
            : public EBusTraits
        {
        public:
            virtual ~FrameCapturePathManagementRequests() = default;

            //! Set the folder where the screenshot will be stored.
            //! @param screenshotFolder The screenshot folder.
            virtual void SetScreenshotFolder(const AZStd::string& screenshotFolder) = 0;

            //! Set the test environment path portion under the screenshot folder, including render API, GPU info, etc.
            //! The full path name is a combination of screenshotFolder + envPath + imageName.
            //! @param envPath Test environment path
            virtual void SetTestEnvPath(const AZStd::string& envPath) = 0;

            //! Set the folder of official baseline images to be compared with the screenshots.
            //! @param baselineFolder The folder of the official baseline images
            virtual void SetOfficialBaselineImageFolder(const AZStd::string& baselineFolder) = 0;

            //! Set the folder of local baseline images to be compared with the screenshots.
            //! @param baselineFolder The folder of the local baseline images
            virtual void SetLocalBaselineImageFolder(const AZStd::string& baselineFolder) = 0;

            //! Build the screenshot file path: screenshotFolder + envPath + imageName
            //! @imageName the image name of the screenshot; when empty string is passed, returns the folder.
            //! @return the full path of the screenshot.
            virtual AZStd::string BuildScreenshotFilePath(const AZStd::string& imageName) = 0;

            //! Build the screenshot file path: officialBaselineImageFolder + imageName
            //! @param imageName the image name of the screenshot; when empty string is passed, returns the folder.
            //! @return the full path of the screenshot.
            virtual AZStd::string BuildOfficialBaselineFilePath(const AZStd::string& imageName) = 0;

            //! Build the screenshot file path: localBaselineImageFolder + envPath + imageName
            //! @param imageName the image name of the screenshot; when empty string is passed, returns the folder.
            //! @return the full path of the screenshot.
            virtual AZStd::string BuildLocalBaselineFilePath(const AZStd::string& imageName) = 0;
        };
        using FrameCapturePathManagementRequestBus = EBus<FrameCapturePathManagementRequests>;

    } // namespace Render

} // namespace AZ
