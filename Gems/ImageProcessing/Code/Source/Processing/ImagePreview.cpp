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

#include <ImageProcessing_precompiled.h>

#include <Processing/ImagePreview.h>
#include <BuilderSettings/BuilderSettingManager.h>
#include <ImageLoader/ImageLoaders.h>
#include <Processing/ImageConvert.h>
#include <Processing/PixelFormatInfo.h>
#include <Editor/EditorCommon.h>
#include <Processing/ImageFlags.h>

#include <AzFramework/StringFunc/StringFunc.h>


namespace ImageProcessing
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
        if (m_presetSetting == nullptr || m_presetSetting->m_uuid != m_textureSetting->m_preset)
        {
            m_presetSetting = BuilderSettingManager::Instance()->GetPreset(m_textureSetting->m_preset);
        }
        const bool isPreview = true;
        const bool autoDelete = false;
        PlatformName defaultPlatform = BuilderSettingManager::s_defaultPlatform;

        m_convertJob = AZStd::make_unique<ImageConvertJob>(m_inputImage, m_textureSetting, m_presetSetting, 
            isPreview, defaultPlatform, &m_output, autoDelete, m_jobContext.get());
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
        return m_output.GetOutputImage(ImageConvertOutput::Preview);
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
        AZ::JobManagerThreadDesc threadDesc;
        desc.m_workerThreads.push_back(threadDesc);
        // Check to ensure these have not already been initialized.
        AZ_Error("Image Processing", !m_jobManager && !m_jobCancelGroup && !m_jobContext, "ImagePreview::InitializeJobSettings is being called again after it has already been initialized");
        m_jobManager = AZStd::make_unique<AZ::JobManager>(desc);
        m_jobCancelGroup = AZStd::make_unique<AZ::JobCancelGroup>();
        m_jobContext = AZStd::make_unique<AZ::JobContext>(*m_jobManager, *m_jobCancelGroup);

        new (&m_doneJob) AZ::JobCompletion(m_jobContext.get()); //re-initialize with the job context
    }


    void GetImageInfoString(IImageObjectPtr image, bool isAlpha, AZStd::string& output)
    {
        if (!image)
        {
            return;
        }

        CPixelFormats& pixelFormats = CPixelFormats::GetInstance();
        EPixelFormat format = image->GetPixelFormat();
        const PixelFormatInfo* info = pixelFormats.GetPixelFormatInfo(format);
        if (info)
        {
            output += AZStd::string::format("Format: %s\r\n", info->szName);
        }

        AZ::u32 mipCount = image->GetMipCount();
        output += AZStd::string::format("Mip Count: %d\r\n", mipCount);

        AZ::u32 memSize = image->GetTextureMemory();
        AZStd::string memSizeString = ImageProcessingEditor::EditorHelper::GetFileSizeString(memSize);
        output += AZStd::string::format("Memory Size: %s\r\n", memSizeString.c_str());

        if (!isAlpha)
        {
            if (image->HasImageFlags(EIF_SRGBRead))
            {
                output += "Color Space: sRGB\r\n";
            }
            else
            {
                output += "Color Space: Linear\r\n";
            }

            if (image->HasImageFlags(EIF_Cubemap))
            {
                output += "Cubemap\r\n";
            }
        }

        AZ::u32 imageFlag = image->GetImageFlags();
        output += AZStd::string::format("Image Flag: 0x%08x\r\n", imageFlag);
    }

    bool ImagePreview::GetProductTexturePreview(const char* fullProductFileName, QImage& previewImage, AZStd::string& productInfo, AZStd::string& productAlphaInfo)
    {
        if (!AzFramework::StringFunc::Path::IsExtension(fullProductFileName, "dds", false))
        {
            return false;
        }

        IImageObjectPtr originImage = IImageObjectPtr(LoadImageFromDdsFile(fullProductFileName));
        IImageObjectPtr alphaImage;

        if (originImage && originImage->HasImageFlags(EIF_AttachedAlpha))
        {
            if (originImage->HasImageFlags(EIF_Splitted))
            {
                AZStd::string alphaFileName = AZStd::string::format("%s.a", fullProductFileName);
                alphaImage = IImageObjectPtr(LoadImageFromDdsFile(alphaFileName));

            }
            else
            {
                alphaImage = IImageObjectPtr(LoadAttachedImageFromDdsFile(fullProductFileName, originImage));
            }
        }

        GetImageInfoString(originImage, false, productInfo);
        GetImageInfoString(alphaImage, true, productAlphaInfo);

        IImageObjectPtr combinedImage = MergeOutputImageForPreview(originImage, alphaImage);
        if (combinedImage)
        {
            AZ::u8* imageBuf;
            AZ::u32 pitch;
            combinedImage->GetImagePointer(0, imageBuf, pitch);
            const AZ::u32 width = originImage->GetWidth(0);
            const AZ::u32 height = originImage->GetHeight(0);
            QImage result = QImage(imageBuf, width, height, pitch, QImage::Format_RGBA8888);
            previewImage = result.copy(); // Return a deep copy here
            return true;
        }

        return false;
    }

}// namespace ImageProcessing
