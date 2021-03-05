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

#include <Processing/ImageConvertJob.h>
#include <AzCore/Jobs/JobCompletion.h>

namespace ImageProcessing
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

        // Output preview image for Asset Browser
        static bool GetProductTexturePreview(const char* fullProductFileName, QImage& previewImage, AZStd::string& productInfo, AZStd::string& productAlphaInfo);

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
        AZStd::unique_ptr<ImageConvertJob> m_convertJob;
        AZ::JobCompletion m_doneJob;
    };
    
    // Get basic image info as string
    void GetImageInfoString(IImageObjectPtr image, bool isAlpha, AZStd::string& output);

}// namespace ImageProcessing
