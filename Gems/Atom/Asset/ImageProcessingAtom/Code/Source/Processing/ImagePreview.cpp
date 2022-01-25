/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <Processing/ImagePreview.h>
#include <BuilderSettings/BuilderSettingManager.h>
#include <ImageLoader/ImageLoaders.h>
#include <Processing/ImageConvert.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/Utils.h>
#include <Editor/EditorCommon.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <QImage>

namespace ImageProcessingAtom
{
    ImagePreview::ImagePreview(const AZStd::string& inputImageFile, TextureSettings* textureSetting)
        : m_imageFileName(inputImageFile)
        , m_textureSetting(textureSetting)
        , m_presetSetting(nullptr)
        , m_inputImage(nullptr)
    {
        InitializeJobSettings();
    }

    void ImagePreview::StartConvert()
    {
        // If there is ongoing job, cancel it
        Cancel();
        m_output.Reset();
        if (m_inputImage == nullptr)
        {
            // Load input image
            m_inputImage = IImageObjectPtr(LoadImageFromFile(m_imageFileName));
        }
        // Get preset if the setting in texture is changed
        if (m_presetSetting == nullptr || m_presetSetting->m_name != m_textureSetting->m_preset)
        {
            m_presetSetting = BuilderSettingManager::Instance()->GetPreset(m_textureSetting->m_preset);
        }
        const bool autoDelete = false;
        PlatformName defaultPlatform = BuilderSettingManager::s_defaultPlatform;

        m_convertJob = AZStd::make_unique<ImagePreviewConvertJob>(m_inputImage, m_textureSetting, m_presetSetting,
            defaultPlatform, &m_output, autoDelete, m_jobContext.get());
        m_convertJob->SetDependent(&m_doneJob);
        m_convertJob->Start();
    }

    bool ImagePreview::IsDone()
    {
        return m_output.IsReady();
    }

    float ImagePreview::GetProgress()
    {
        if (!m_output.IsReady())
        {
            return m_output.GetProgress();
        }
        return 1.0f;
    }

    void ImagePreview::Cancel()
    {
        if (m_convertJob)
        {
            m_convertJob->Cancel();
            // Block until job completes
            m_doneJob.StartAndWaitForCompletion();

            AZ_Assert(m_output.IsReady(), "Conversion job is not done yet!");
        }
        m_convertJob.release();
        m_doneJob.Reset(true);
    }

    IImageObjectPtr ImagePreview::GetOutputImage()
    {
        return m_output.GetOutputImage();
    }

    ImagePreview::~ImagePreview()
    {
        Cancel();
        // Maintain the releasing order
        m_jobManager.release();
        m_jobContext.release();
        m_jobCancelGroup.release();
    }

    void ImagePreview::InitializeJobSettings()
    {
        AZ::JobManagerDesc desc;
        desc.m_jobManagerName = "ImagePreview";

        AZ::JobManagerThreadDesc threadDesc;
        desc.m_workerThreads.push_back(threadDesc);
        // Check to ensure these have not already been initialized.
        AZ_Error("Image Processing", !m_jobManager && !m_jobCancelGroup && !m_jobContext, "ImagePreview::InitializeJobSettings is being called again after it has already been initialized");
        m_jobManager = AZStd::make_unique<AZ::JobManager>(desc);
        m_jobCancelGroup = AZStd::make_unique<AZ::JobCancelGroup>();
        m_jobContext = AZStd::make_unique<AZ::JobContext>(*m_jobManager, *m_jobCancelGroup);

        new (&m_doneJob) AZ::JobCompletion(m_jobContext.get()); //re-initialize with the job context
    }

}// namespace ImageProcessingAtom
