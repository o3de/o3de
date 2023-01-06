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
        class FrameCaptureTestRequests
            : public EBusTraits
        {
        public:
            virtual ~FrameCaptureTestRequests() = default;

            //! Set the folder where the screenshot will be stored.
            //! @param screenshotFolder The screenshot folder.
            virtual void SetScreenshotFolder(const AZStd::string& screenshotFolder) = 0;

            //! Set the test environment path portion under the screenshot folder, including render API, GPU info, etc.
            //! The full path name is a combination of screenshotFolder + envPath + imageName.
            //! @param envPath Test environment path
            virtual void SetTestEnvPath([[maybe_unused]] const AZStd::string& envPath){};

            //! Set the folder of official baseline images to be compared with the screenshots.
            //! @param baselineFolder The folder of the official baseline images
            virtual void SetOfficialBaselineImageFolder([[maybe_unused]] const AZStd::string& baselineFolder){};

            //! Set the folder of local baseline images to be compared with the screenshots.
            //! @param baselineFolder The folder of the local baseline images
            virtual void SetLocalBaselineImageFolder([[maybe_unused]] const AZStd::string& baselineFolder){};

            //! Build the screenshot file path: screenshotFolder + envPath + imageName
            //! @imageName the image name of the screenshot; when empty string is passed, returns the folder.
            //! @return the full path of the screenshot.
            virtual AZStd::string BuildScreenshotFilePath(const AZStd::string& imageName, bool useEnvPath) = 0;

            //! Build the screenshot file path: officialBaselineImageFolder + imageName
            //! @param imageName the image name of the screenshot; when empty string is passed, returns the folder.
            //! @return the full path of the screenshot.
            virtual AZStd::string BuildOfficialBaselineFilePath(const AZStd::string& imageName, bool useEnvPath) = 0;

            //! Build the screenshot file path: localBaselineImageFolder + envPath + imageName
            //! @param imageName the image name of the screenshot; when empty string is passed, returns the folder.
            //! @return the full path of the screenshot.
            virtual AZStd::string BuildLocalBaselineFilePath(const AZStd::string& imageName, bool useEnvPath) = 0;

            //! Compare 2 screenshots files and give scores (using root mean square RMS) for the difference.
            //! @param filePathA the full path of screenshot A
            //! @param filePathB the full path of screenshot B
            //! @param minDiffFilter diff values less than this will be filtered out before calculating ImageDiffResult::m_filteredDiffScore.
            //! @return the result code, diff score and filtered diff score.
            virtual Utils::ImageDiffResult CompareScreenshots(
                const AZStd::string& filePathA,
                const AZStd::string& filePathB,
                float minDiffFilter) = 0;
        };
        using FrameCaptureTestRequestBus = EBus<FrameCaptureTestRequests>;

    } // namespace Render

} // namespace AZ
