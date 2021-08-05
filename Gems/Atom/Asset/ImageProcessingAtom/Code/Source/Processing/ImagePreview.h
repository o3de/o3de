/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Processing/ImageConvertJob.h>
#include <AzCore/Jobs/JobCompletion.h>

class QImage;

namespace ImageProcessingAtom
{
    // The reason to have image preview class, we should keep the source image loaded once,
    // we could restart conversion and cancel the old conversion at anytime when setting changed
    class ImagePreview
    {
    public:
        ImagePreview(const AZStd::string& inputImageFile, TextureSettings* textureSetting);
        ~ImagePreview();

        void InitializeJobSettings();
        void StartConvert();
        bool IsDone();
        float GetProgress();
        void Cancel();
        IImageObjectPtr GetOutputImage();

    private:
        AZStd::string m_imageFileName;
        IImageObjectPtr m_inputImage;
        const TextureSettings* m_textureSetting;
        const PresetSettings* m_presetSetting;

        IImageObjectPtr m_outputImage;
        IImageObjectPtr m_outputAlphaImage;

        ImageConvertOutput m_output;

        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::unique_ptr<AZ::JobCancelGroup> m_jobCancelGroup;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;
        AZStd::unique_ptr<ImagePreviewConvertJob> m_convertJob;
        AZ::JobCompletion m_doneJob;
    };
    
}// namespace ImageProcessingAtom
