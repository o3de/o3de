/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Processing/ImageConvertJob.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/ImageToProcess.h>
#include <Converters/PixelOperation.h>
#include <Processing/ImageConvert.h>
#include <BuilderSettings/BuilderSettingManager.h>

namespace ImageProcessingAtom
{
    IImageObjectPtr ImageConvertOutput::GetOutputImage(OutputImageType type) const
    {
        if (type < OutputImageType::Count)
        {
            return m_outputImage[static_cast<int>(type)];
        }
        else
        {
            return IImageObjectPtr();
        }
    }

    void ImageConvertOutput::SetOutputImage(IImageObjectPtr image, OutputImageType type)
    {
        if (type < OutputImageType::Count)
        {
            m_outputImage[static_cast<int>(type)] = image;
        }
        else
        {
            AZ_Error("ImageProcess", false, "Cannot set output image to %d", type);
        }
    }

    void ImageConvertOutput::SetReady(bool ready)
    {
        m_outputReady = ready;
    }

    bool ImageConvertOutput::IsReady() const
    {
        return m_outputReady;
    }

    float ImageConvertOutput::GetProgress() const
    {
        return m_progress;
    }

    void ImageConvertOutput::SetProgress(float progress)
    {
        m_progress = progress;
    }

    void ImageConvertOutput::Reset()
    {
        for (int i = 0; i < static_cast<int>(OutputImageType::Count); i++)
        {
            m_outputImage[i] = nullptr;
        }
        m_outputReady = false;
        m_progress = 0.0f;
    }

    ImagePreviewConvertJob::ImagePreviewConvertJob(IImageObjectPtr image, const TextureSettings* textureSetting,
        const PresetSettings* preset, const AZStd::string& platformId,
        ImageConvertOutput* output, bool autoDelete /*= true*/, AZ::JobContext* jobContext /*= nullptr*/)
        : AZ::Job(autoDelete, jobContext)
        , m_isCancelled(false)
        , m_output(output)
    {
        AZ_Assert(m_output, "Needs to have an output destination for image conversion!");
        if (image && textureSetting && preset)
        {
            AZStd::unique_ptr<ImageConvertProcessDescriptor> desc = AZStd::make_unique<ImageConvertProcessDescriptor>();
            desc->m_textureSetting = *textureSetting;
            desc->m_presetSetting = *preset;
            desc->m_platform = platformId;
            desc->m_inputImage = image;
            desc->m_isPreview = true;

            m_process = AZStd::make_unique<ImageConvertProcess>(AZStd::move(desc));
        }
    }

    void ImagePreviewConvertJob::Process()
    {
        if (!m_process)
        {
            AZ_Error("Image Processing", false, "Cannot start processing, invalid setting or image!");
            m_output->SetReady(true);
            m_output->SetProgress(1.0f);
            return;
        }
        m_output->SetReady(false);
        while (!m_process->IsFinished() && !IsJobCancelled())
        {
            m_process->UpdateProcess();
            m_output->SetProgress(m_process->GetProgress() / static_cast<float>(m_previewProcessStep));
        }

        IImageObjectPtr outputImage = m_process->GetOutputImage();
        IImageObjectPtr outputImageAlpha = m_process->GetOutputAlphaImage();

        m_output->SetOutputImage(outputImage, ImageConvertOutput::Base);
        m_output->SetOutputImage(outputImageAlpha, ImageConvertOutput::Alpha);

        if (!IsJobCancelled())
        {
            // For preview, combine image output with alpha if any
            m_output->SetProgress(1.0f / static_cast<float>(m_previewProcessStep));
            IImageObjectPtr combinedImage = MergeOutputImageForPreview(outputImage, outputImageAlpha);
            m_output->SetOutputImage(combinedImage, ImageConvertOutput::Preview);
        }

        m_output->SetReady(true);
        m_output->SetProgress(1.0f);
    }

    void ImagePreviewConvertJob::Cancel()
    {
        m_isCancelled = true;
    }

    bool ImagePreviewConvertJob::IsJobCancelled()
    {
        return m_isCancelled || IsCancelled();
    }
}// namespace ImageProcessingAtom
