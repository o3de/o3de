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

#include <BuilderSettings/ImageProcessingDefines.h>
#include <BuilderSettings/PresetSettings.h>
#include <BuilderSettings/TextureSettings.h>
#include <ImageProcessing/ImageObject.h>
#include <AzCore/Jobs/Job.h>

namespace ImageProcessing
{
    class ImageConvertProcess;

    class ImageConvertOutput
    {
    public:
        enum OutputImageType
        {
            Base = 0,   // Might contains alpha or not
            Alpha,      // Separate alpha image
            Preview,    // Combine base image with alpha if any, format RGBA8
            Count
        };

        IImageObjectPtr GetOutputImage(OutputImageType type) const;
        void SetOutputImage(IImageObjectPtr image, OutputImageType type);
        void SetReady(bool ready);
        bool IsReady() const;
        float GetProgress() const;
        void SetProgress(float progress);
        void Reset();

    private:
        IImageObjectPtr m_outputImage[OutputImageType::Count];
        bool m_outputReady = false;
        float m_progress = 0.0f;
    };

    class ImageConvertJob
        : public AZ::Job
    {
    public:
        AZ_CLASS_ALLOCATOR(ImageConvertJob, AZ::ThreadPoolAllocator, 0)

        ImageConvertJob(IImageObjectPtr image, const TextureSettings* textureSetting, const PresetSettings* preset
            , bool isPreview, const AZStd::string& platformId, ImageConvertOutput* output, bool autoDelete = true
            , AZ::JobContext* jobContext = nullptr);

        void Process() override;
        // Cancel the job itself
        void Cancel();
        // Whether the job is being cancelled or the whole job group is being cancelled
        bool IsJobCancelled();

    private:
        static const int m_previewProcessStep = 2;

        AZStd::unique_ptr<ImageConvertProcess> m_process;
        bool m_isPreview;
        AZStd::atomic_bool m_isCancelled;
        ImageConvertOutput* m_output;
    };
}// namespace ImageProcessing
