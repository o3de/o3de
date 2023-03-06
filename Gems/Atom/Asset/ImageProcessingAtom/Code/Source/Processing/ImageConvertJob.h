/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuilderSettings/PresetSettings.h>
#include <BuilderSettings/TextureSettings.h>
#include <Atom/ImageProcessing/ImageObject.h>
#include <Atom/ImageProcessing/ImageProcessingDefines.h>
#include <AzCore/Jobs/Job.h>

namespace ImageProcessingAtom
{
    class ImageConvertProcess;

    class ImageConvertOutput
    {
    public:
        IImageObjectPtr GetOutputImage() const;
        void SetOutputImage(IImageObjectPtr image);
        void SetReady(bool ready);
        bool IsReady() const;
        float GetProgress() const;
        void SetProgress(float progress);
        void Reset();

    private:
        IImageObjectPtr m_outputImage;
        bool m_outputReady = false;
        float m_progress = 0.0f;
    };

    /**
     * The ImagePreviewConvertJob is the multi-thread job to enable ImageConvertProcessing running on job thread for image process
     * result preview window. This is used for generate preview result only.
     * Note, we don't need this for image builder of asset processor since the job for image builder is managed by the builder system.
     */
    class ImagePreviewConvertJob
        : public AZ::Job
    {
    public:
        AZ_CLASS_ALLOCATOR(ImagePreviewConvertJob, AZ::ThreadPoolAllocator)

        ImagePreviewConvertJob(IImageObjectPtr image, const TextureSettings* textureSetting, const PresetSettings* preset
            , const AZStd::string& platformId, ImageConvertOutput* output, bool autoDelete = true
            , AZ::JobContext* jobContext = nullptr);

        void Process() override;
        // Cancel the job itself
        void Cancel();
        // Whether the job is being canceled or the whole job group is being canceled
        bool IsJobCancelled();

    private:
        static const int m_previewProcessStep = 2;

        AZStd::unique_ptr<ImageConvertProcess> m_process;
        AZStd::atomic_bool m_isCancelled;
        ImageConvertOutput* m_output;
    };
}// namespace ImageProcessingAtom
