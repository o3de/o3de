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

#include <Processing/PixelFormatInfo.h>
#include <Processing/ImageToProcess.h>
#include <Processing/ImageConvert.h>
#include <Converters/FIR-Weights.h>
#include <Converters/Cubemap.h>
#include <Converters/PixelOperation.h>
#include <Converters/Histogram.h>
#include <ImageLoader/ImageLoaders.h>
#include <BuilderSettings/BuilderSettingManager.h>
#include <BuilderSettings/PresetSettings.h>
#include <Processing/ImageFlags.h>

#include <AzFramework/StringFunc/StringFunc.h>

//qt has convenience functions to handle file
#include <qfile.h>
#include <qdir.h>

//for texture splitting
//mininum number of low level mips will be saved in the base file.
#define MinPersistantMips 3
//mininum texture size to be splitted. A texture will only be split when the size is larger than this number
#define MinSizeToSplit 1<<5

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
    #if defined(TOOLS_SUPPORT_XENIA)
        #include AZ_RESTRICTED_FILE_EXPLICIT(ImageProcess, Xenia)
    #endif
    #if defined(TOOLS_SUPPORT_JASPER)
        #include AZ_RESTRICTED_FILE_EXPLICIT(ImageProcess, Jasper)
    #endif
    #if defined(TOOLS_SUPPORT_PROVO)
        #include AZ_RESTRICTED_FILE_EXPLICIT(ImageProcess, Provo)
    #endif
    #if defined(TOOLS_SUPPORT_SALEM)
        #include AZ_RESTRICTED_FILE_EXPLICIT(ImageProcess, Salem)
    #endif
#endif

namespace ImageProcessing
{

    IImageObjectPtr ImageConvertProcess::GetOutputImage()
    {
        if (m_image)
        {
            return m_image->Get();
        }
        return nullptr;
    }

    IImageObjectPtr ImageConvertProcess::GetOutputAlphaImage()
    {
        return m_alphaImage;
    }
    
    IImageObjectPtr ImageConvertProcess::GetOutputDiffCubemap()
    {
        return m_diffCubemapImage;
    }
    
    void ImageConvertProcess::GetAppendOutputFilePaths(AZStd::vector<AZStd::string>& outPaths)
    {
        for (const auto& path : m_productFilepaths)
        {
            outPaths.push_back(path);
        }
    }

    ImageConvertProcess::ImageConvertProcess(const IImageObjectPtr inputImage, const TextureSettings& textureSetting,
        const PresetSettings& presetSetting, bool isPreview, bool isStreaming, bool canOverridePreset, 
        const AZStd::string& outputPath, const AZStd::string& platformId) :
        m_inputImage(inputImage),
        m_textureSetting(textureSetting),
        m_presetSetting(presetSetting),
        m_canOverridePreset(canOverridePreset),
        m_image(nullptr),
        m_isPreview(isPreview),
        m_outputPath(outputPath),
        m_progressStep(0),
        m_isFinished(false),
        m_isSucceed(false),
        m_processTime(0),
        m_isStreaming(isStreaming),
        m_platformId(platformId)
    {
    }

    ImageConvertProcess::~ImageConvertProcess()
    {
        delete m_image;
    }

    bool ImageConvertProcess::IsConvertToCubemap()
    {
        return m_presetSetting.m_cubemapSetting != nullptr;
    }
    
    void ImageConvertProcess::UpdateProcess()
    {
        if (m_isFinished)
        {
            return;
        }

        switch (m_progressStep)
        {
        case StepValidateInput:
            //validate 
            if (!ValidateInput())
            {
                m_isSucceed = false;
                break;
            }

            //set start time      
            m_startTime = AZStd::GetTimeUTCMilliSecond();

            //identify the alpha content of input image if gloss from normal wasn't set
            m_alphaContent = m_inputImage->GetAlphaContent();

            //create image for process
            m_image = new ImageToProcess(IImageObjectPtr(m_inputImage->Clone()));

            break;
        case StepGenerateColorChart:
            //GenerateColorChart. 
            if (m_presetSetting.m_isColorChart)
            {
                m_image->CreateColorChart();
            }
            break;
        case StepConvertToLinear:
            //convert to linear space and the output image pixel format should be rgba32f
            ConvertToLinear();
            break;
        case StepSwizzle:
            //convert texture format.
            if (m_presetSetting.m_swizzle.size() >= 4)
            {
                m_image->Get()->Swizzle(m_presetSetting.m_swizzle.substr(0, 4).c_str());
                m_alphaContent = m_image->Get()->GetAlphaContent();
            }

            //convert gloss map (alhpa channel) from legacy distribution to new one
            if (m_presetSetting.m_isLegacyGloss)
            {
                m_image->Get()->ConvertLegacyGloss();
            }
            break;
        case StepOverridePreset:
            if (m_canOverridePreset)
            {
                // Set the pixel format to BC3 if the source contains greyscale alpha and BC1 if it does not.
                if (m_presetSetting.m_pixelFormat == ePixelFormat_BC1
                    || m_presetSetting.m_pixelFormat == ePixelFormat_BC1a
                    || m_presetSetting.m_pixelFormat == ePixelFormat_BC3)
                {
                    if (m_alphaContent == EAlphaContent::eAlphaContent_Greyscale)
                    {
                        m_presetSetting.m_pixelFormat = ePixelFormat_BC3;
                    }
                    else if (m_alphaContent == EAlphaContent::eAlphaContent_OnlyBlackAndWhite
                        || m_alphaContent == EAlphaContent::eAlphaContent_OnlyBlack
                        || m_alphaContent == EAlphaContent::eAlphaContent_OnlyWhite)
                    {
                        m_presetSetting.m_pixelFormat = ePixelFormat_BC1a;
                    }
                    else
                    {
                        m_presetSetting.m_pixelFormat = ePixelFormat_BC1;
                    }
                }
            }
            break;
        case StepCubemapLayout:
            //convert cubemap image's layout to vertical strip used in game. 
            if (IsConvertToCubemap())
            {
                if (!m_image->ConvertCubemapLayout(CubemapLayoutVertical))
                {
                    m_image->Set(nullptr);
                }
            }
            break;
        case StepPreNormalize:
            //normalize base image before mipmap generation if glossfromnormals is enabled and require normalize
            if (m_presetSetting.m_isMipRenormalize && m_presetSetting.m_glossFromNormals)
            {
                // Normalize the base mip map. This has to be done explicitly because we need to disable mip renormalization to
                // preserve the normal length when deriving the normal variance
                m_image->Get()->NormalizeVectors(0, 1);
            }
            break;
        case StepDiffCubemap:
            //create diffuse cubemap. We need to have better way to handle one input multiple export settings later. 
            CreateDiffuseCubemap();
            break;
        case StepMipmap:
            //generate mipmaps 
            if (IsConvertToCubemap())
            {
                FillCubemapMipmaps();
            }
            else
            {
                FillMipmaps();
            }
            //add image flag
            if (m_presetSetting.m_suppressEngineReduce || m_textureSetting.m_suppressEngineReduce)
            {
                m_image->Get()->AddImageFlags(EIF_SupressEngineReduce);
            }
            break;
        case StepGlossFromNormal:
            //get gloss from normal for all mipmaps and save to alpha channel
            if (m_presetSetting.m_glossFromNormals)
            {
                bool hasAlpha = (m_alphaContent == EAlphaContent::eAlphaContent_OnlyBlack
                    || m_alphaContent == EAlphaContent::eAlphaContent_OnlyBlackAndWhite 
                    || m_alphaContent == EAlphaContent::eAlphaContent_Greyscale);

                m_image->Get()->GlossFromNormals(hasAlpha);
                //set alpha content so it won't be ignored later.
                m_alphaContent = EAlphaContent::eAlphaContent_Greyscale;
            }
            break;
        case StepPostNormalize:
            //normalize all the other mipmaps
            if (!IsConvertToCubemap() && m_presetSetting.m_isMipRenormalize)
            {
                if (m_presetSetting.m_glossFromNormals)
                {
                    //normalize other mips except first mip
                    m_image->Get()->NormalizeVectors(1, 100);
                }
                else
                {
                    //normalize all mips
                    m_image->Get()->NormalizeVectors(0, 100);
                }

                m_image->Get()->AddImageFlags(EIF_RenormalizedTexture);
            }
            break;
        case StepCreateHighPass:
            if (m_presetSetting.m_highPassMip > 0)
            {
                m_image->CreateHighPass(m_presetSetting.m_highPassMip);
            }
            break;
        case StepConvertOutputColorSpace:
            //comvert image from linear space to desired output color space
            ConvertToOuputColorSpace();
            break;
        case StepAlphaImage:
            //save alpha channel to separate image if it's needed
            CreateAlphaImage();
            break;
        case StepConvertPixelFormat:
            //convert pixel format
            ConvertPixelformat();
            break;
        case StepSaveToFile:
            //save to file
            if (!m_isPreview)
            {
                m_isSucceed = SaveOutput();
            }
            else
            {
                m_isSucceed = true;
            }
            break;
        }

        m_progressStep++;

        if (m_image == nullptr || m_image->Get() == nullptr || m_progressStep >= StepAll)
        {
            m_isFinished = true;
            AZStd::sys_time_t endTime = AZStd::GetTimeUTCMilliSecond();
            m_processTime = aznumeric_cast<double>((endTime - m_startTime) / 1000);
        }
        
        //output conversion log
        if (m_isSucceed && m_isFinished)
        {
            const uint32 sizeTotal = m_image->Get()->GetTextureMemory();
            if (m_isPreview)
            {
                AZ_TracePrintf("Image Processing", "Image ( %d bytes) converted in %f seconds\n", sizeTotal, m_processTime);
            }
            else
            {
                AZ_TracePrintf("Image Processing", "Image converted and saved to %s ( %d bytes) with %f seconds\n", m_outputPath.c_str(),
                    sizeTotal, m_processTime);
            }
        }
    }

    void ImageConvertProcess::ProcessAll()
    {
        while (!m_isFinished)
        {
            UpdateProcess();
        }
    }

    float ImageConvertProcess::GetProgress()
    {
        return m_progressStep / (float)StepAll;
    }

    bool ImageConvertProcess::IsFinished()
    {
        return m_isFinished;
    }

    bool ImageConvertProcess::IsSucceed()
    {
        return m_isSucceed;
    }
    
    //function to get desired output image extent
    void GetOutputExtent(AZ::u32 inputWidth, AZ::u32 inputHeight, AZ::u32& outWidth, AZ::u32& outHeight, AZ::u32& outReduce,
        const TextureSettings* textureSettings, const PresetSettings* presetSettings)
    {
        outWidth = inputWidth;
        outHeight = inputHeight;
        outReduce = 0;

        if (textureSettings == nullptr || presetSettings == nullptr)
        {
            return;
        }

        //don't do any reduce for color chart
        if (presetSettings->m_isColorChart)
        {
            return;
        }

        //get suitable size for dest pixel format
        CPixelFormats::GetInstance().GetSuitableImageSize(presetSettings->m_pixelFormat, inputWidth, inputHeight,
            outWidth, outHeight);
            
        //desired reduce level. 1 means reduce one level
        uint sizeReduceLevel = textureSettings->m_sizeReduceLevel;

        outReduce = 0;
            
        //reduce to not exceed max texture size
        if (presetSettings->m_maxTextureSize > 0)
        {
            while (outWidth > presetSettings->m_maxTextureSize || outHeight > presetSettings->m_maxTextureSize)
            {
                outWidth >>= 1;
                outHeight >>= 1;
                outReduce++;
            }
        }

        //if it requires to reduce more and the result size will still larger than min texture size, then reduce
        while (outReduce < sizeReduceLevel &&
            (outWidth >= presetSettings->m_minTextureSize * 2 && outHeight >= presetSettings->m_minTextureSize * 2))
        {
            outWidth >>= 1;
            outHeight >>= 1;
            outReduce++;
        }
    }

    bool ImageConvertProcess::ConvertToLinear()
    {
        //de-gamma only if the input is sRGB. this will convert other uncompressed format to RGBA32F
        return m_image->GammaToLinearRGBA32F(m_presetSetting.m_srcColorSpace == ColorSpace::sRGB);
    }

    //mipmap generation
    bool ImageConvertProcess::FillMipmaps()
    {
        //this function only works with pixel format rgba32f 
        const EPixelFormat srcPixelFormat = m_image->Get()->GetPixelFormat();
        if (srcPixelFormat != ePixelFormat_R32G32B32A32F)
        {
            AZ_Assert(false, "%s only works with pixel format rgba32f", __FUNCTION__);
            return false;
        }
            
        //only if the src image has one mip
        if (m_image->Get()->GetMipCount() != 1)
        {
            AZ_Assert(false, "%s called for a mipmapped image. ", __FUNCTION__);
            return false;
        }

        //get output image size
        uint32 outWidth;
        uint32 outHeight;
        uint32 outReduce = 0;
        GetOutputExtent(m_image->Get()->GetWidth(0), m_image->Get()->GetHeight(0), outWidth, outHeight, outReduce, &m_textureSetting,
            &m_presetSetting);
        
        //max mipmap count
        uint32 mipCount = UINT32_MAX;
        if (m_presetSetting.m_mipmapSetting == nullptr || !m_textureSetting.m_enableMipmap)
        {
            mipCount = 1;
        }

        //create new new output image with proper side
        IImageObjectPtr outImage(IImageObject::CreateImage(outWidth, outHeight, mipCount, ePixelFormat_R32G32B32A32F));
        
        //filter setting for mip map generation
        float blurH = 0;
        float blurV = 0;

        //fill mipmap data for uncompressed output image
        for (uint32 mip = 0; mip < outImage->GetMipCount(); mip++)
        {
            FilterImage(m_textureSetting.m_mipGenType, m_textureSetting.m_mipGenEval, blurH, blurV, m_image->Get(), 0, outImage, mip, nullptr, nullptr);
        }        

        //transfer alpha coverage
        if (m_textureSetting.m_maintainAlphaCoverage)
        {
            outImage->TransferAlphaCoverage(&m_textureSetting, m_image->Get());
        }

        //set back to image
        m_image->Set(outImage);
        return true;
    }

    void ImageConvertProcess::CreateAlphaImage()
    {
        //if alpha content doesn't have alpha or we need to discard alpha, skip
        //we won't create alpha image for cubemap too
        if (m_alphaContent == EAlphaContent::eAlphaContent_Absent
            || m_alphaContent == EAlphaContent::eAlphaContent_OnlyWhite
            || m_presetSetting.m_discardAlpha || IsConvertToCubemap())
        {
            return;
        }

        //Ensure that the PixelFormatAlpha is set otherwise no need to create m_alphaImage
        if (m_presetSetting.m_pixelFormatAlpha == ePixelFormat_Unknown)
        {
            return;
        }

        //now create alpha image
        ImageToProcess alphaImage(m_image->Get());
        alphaImage.ConvertFormat(ePixelFormat_A8);
        
        
        if(CPixelFormats::GetInstance().IsFormatSingleChannel(m_presetSetting.m_pixelFormatAlpha))
        {
            alphaImage.ConvertFormat(m_presetSetting.m_pixelFormatAlpha);
        }
        else
        {
            //For PVRTC compression we need to clear out the alpha to get accurate rgb compression.
            if (IsPVRTCFormat(m_presetSetting.m_pixelFormat) || IsASTCFormat(m_presetSetting.m_pixelFormat))
            {
                alphaImage.ConvertFormat(ePixelFormat_R8G8B8A8);
                alphaImage.Get()->Swizzle("rgb1");
                alphaImage.ConvertFormat(m_presetSetting.m_pixelFormatAlpha);
            }
            else
            {
                AZ_Assert(false, "Did you apply the correct pixel format for PixelFormatAlpha?");
            }
        }
        
        //get final result and save it to member variable for later use
        m_alphaImage = alphaImage.Get();

        m_image->Get()->AddImageFlags(EIF_AttachedAlpha);
    }

    //pixel format convertions
    bool ImageConvertProcess::ConvertPixelformat()
    {

        //For PVRTC compression we need to clear out the alpha to get accurate rgb compression.
        if(m_alphaImage && (IsPVRTCFormat(m_presetSetting.m_pixelFormat) || IsASTCFormat(m_presetSetting.m_pixelFormat)))
        {
            m_image->Get()->Swizzle("rgb1");
        }


        //set up compress option
        ICompressor::EQuality quality;
        if (m_isPreview)
        {
            quality = ICompressor::eQuality_Preview;
        }
        else
        {
            quality = ICompressor::eQuality_Normal;
        }
        m_image->GetCompressOption().compressQuality = quality;
        m_image->GetCompressOption().rgbWeight = m_presetSetting.GetColorWeight();
        m_image->ConvertFormat(m_presetSetting.m_pixelFormat);
        return true;
    }    
    
    //convert color space from linear to sRGB space if it's neccessary
    bool ImageConvertProcess::ConvertToOuputColorSpace()
    {
        if (m_presetSetting.m_destColorSpace == ColorSpace::sRGB)
        {
            m_image->LinearToGamma();
        }
        else if (m_presetSetting.m_destColorSpace == ColorSpace::autoSelect)
        {
            //convert to sRGB color space if it's dark image (converting bright images decreases image quality)
            bool bThresholded = false;
            {
                Histogram<256> histogram;
                if (ComputeLuminanceHistogram(m_image->Get(), histogram))
                {
                    const size_t medianBinIndex = 116;
                    float percentage = histogram.getPercentage(medianBinIndex, 255);

                    // The image has significant amount of dark pixels, it's good to use sRGB
                    bThresholded = (percentage < 50.0f);
                }
            }

            if (bThresholded)
            {
                bool convertToSRGB = true;

                // if the image is BC1 compressable, additionally estimate the conversion error
                // to only convert if it doesn't introduce error
                if (CPixelFormats::GetInstance().IsImageSizeValid(ePixelFormat_BC1, m_image->Get()->GetWidth(0),
                    m_image->Get()->GetHeight(0), false))
                {
                    //get image in RGB space
                    ImageToProcess imageProcess(m_image->Get());
                    imageProcess.LinearToGamma();

                    ICompressor::CompressOption option;
                    option.compressQuality = ICompressor::eQuality_Preview;
                    option.rgbWeight = m_presetSetting.GetColorWeight();

                    float errorLinearBC1;
                    float errorSrgbBC1;
                    GetBC1CompressionErrors(m_image->Get(), errorLinearBC1, errorSrgbBC1, option);
                    
                    // Don't convert if it would lower the image quality when saved as sRGB according to GetDXT1GammaCompressionError()
                    if (errorSrgbBC1 >= errorLinearBC1)
                    {
                        convertToSRGB = false;
                    }
                }

                // our final conclusion: if the texture had a significant percentage of dark pixels and, 
                // if applicable, it was BC1 compressable and gamma compression wouldn't introduce error,
                // then we convert it to sRGB
                if (convertToSRGB)
                {
                    m_image->LinearToGamma();
                }
            }
        }
        return true;
    }

    bool ImageConvertProcess::ValidateInput()
    {
        //valid the input image and output settings here. 
        uint32 dwWidth, dwHeight;
        dwWidth = m_inputImage->GetWidth(0);
        dwHeight = m_inputImage->GetHeight(0);

        EPixelFormat dstFmt = m_presetSetting.m_pixelFormat;

        //check if whether input image can be a cubemap 
        if (m_presetSetting.m_cubemapSetting)
        {
            if (CubemapLayout::GetCubemapLayoutInfo(m_inputImage) == nullptr)
            {
                AZ_Error("Image Processing", false, "Invalid image size %dx%d using as cubemap. Requires power of two with 6x1, 1x6, 4x3 or 3x4 layouts", dwWidth, dwHeight);
                return false;
            }
        }
        else if (!CPixelFormats::GetInstance().IsImageSizeValid(dstFmt, dwWidth, dwHeight, false))
        {
            AZ_Warning("Image Processing", false, "Image size will be scaled for pixel format %s", CPixelFormats::GetInstance().GetPixelFormatInfo(dstFmt)->szName);
        }
        
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        if (ImageProcess##PrivateName::DoesSupport(m_platformId))\
        {\
            if(!ImageProcess##PrivateName::IsPixelFormatSupported(m_presetSetting.m_pixelFormat))\
            {\
                AZ_Error("Image Processing", false, "Unsupported pixel format %s for %s",\
                    CPixelFormats::GetInstance().GetPixelFormatInfo(dstFmt)->szName, m_platformId.c_str());\
                return false;\
            }\
        }
        AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif //AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS

        return true;
    }

    bool ImageConvertProcess::SaveOutput()
    {
        //if the path wasn't specified, skip
        if (m_outputPath.empty())
        {
            AZ_Error("Image Processing", false, "No output path provided for saving");
            return false;
        }

        //set all mips as presistent mips by default. it will be modified if the image is splitted later
        m_image->Get()->SetNumPersistentMips(m_image->Get()->GetMipCount());

        //split
        if (m_isStreaming && m_presetSetting.m_numStreamableMips > 0)
        {
            IImageObjectPtr curImage = m_image->Get();
            
            if (curImage->GetMipCount() > MinPersistantMips && (curImage->GetWidth(0) > MinSizeToSplit ||
                curImage->GetWidth(0) > MinSizeToSplit))
            {
                //calculate final persistance mip count 
                AZ::u32 persistantMips = MinPersistantMips;
                if (m_presetSetting.m_numStreamableMips < curImage->GetMipCount() - MinPersistantMips)
                {
                    persistantMips = curImage->GetMipCount() - m_presetSetting.m_numStreamableMips;
                }
                curImage->SetNumPersistentMips(persistantMips);
                curImage->AddImageFlags(EIF_Splitted);

                //add flags for alpha image too, assuming alpha image has same size as origin
                if (m_alphaImage)
                {
                    m_alphaImage->SetNumPersistentMips(persistantMips);
                    m_alphaImage->AddImageFlags(EIF_Splitted);
                }
            }
        }

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        if (ImageProcess##PrivateName::DoesSupport(m_platformId))\
        {\
            ImageProcess##PrivateName::PrepareImageForExport(m_image->Get());\
            ImageProcess##PrivateName::PrepareImageForExport(m_alphaImage);\
        }
        AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif //AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS

        AZStd::vector<AZStd::string> outputFilePaths;
        if (!m_image->Get()->SaveImage(m_outputPath.c_str(), m_alphaImage, outputFilePaths))
        {
            AZ_Error("Image Processing", false, "Save image to %s failed", m_outputPath.c_str());
            return false;
        }

        for (auto& path : outputFilePaths)
        {
            m_productFilepaths.push_back(path);
        }
        return true;
    }

    ImageConvertProcess* CreateImageConvertProcess(const AZStd::string& imageFilePath, const AZStd::string& exportDir
        , const PlatformName& platformName, AZ::SerializeContext* context)
    {
        AZStd::string metafilePath;
        BuilderSettingManager::Instance()->MetafilePathFromImagePath(imageFilePath, metafilePath);
        TextureSettings textureSettings;

        MultiplatformTextureSettings multiTextureSetting;
        bool canOverridePreset = false;
        
        multiTextureSetting = TextureSettings::GetMultiplatformTextureSetting(imageFilePath, canOverridePreset, context);
        if (multiTextureSetting.empty())
        {
            AZ_Error("Image Processing", false, "Could not determine export settings for image file [%s] due to previous error(s). Skipping export...", imageFilePath.c_str());
            return nullptr;
        }

        if (multiTextureSetting.find(platformName) != multiTextureSetting.end())
        {
            textureSettings = multiTextureSetting[platformName];
        }
        else
        {
            PlatformName defaultPlatform = BuilderSettingManager::s_defaultPlatform;
            if (multiTextureSetting.find(defaultPlatform) != multiTextureSetting.end())
            {
                textureSettings = multiTextureSetting[defaultPlatform];
            }
            else
            {
                textureSettings = (*multiTextureSetting.begin()).second;
            }
        }

        //load image. Do it earlier so GetSuggestedPreset function could use the information of file to choose better preset
        IImageObjectPtr srcImage(LoadImageFromFile(imageFilePath));
        if (srcImage == nullptr)
        {
            AZ_Error("Image Processing", false, "Load image file %s failed", imageFilePath.c_str());
            return nullptr;
        }

        //if get textureSetting failed, use the default texture setting, and find suitable preset for this file
        //in very rare user case, an old texture setting file may not have a preset. We fix it over here too. 
        if (textureSettings.m_preset.IsNull())
        {
            textureSettings.m_preset = BuilderSettingManager::Instance()->GetSuggestedPreset(imageFilePath, srcImage);
        }

        //get preset
        const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(textureSettings.m_preset, platformName);

        if (preset == nullptr)
        {
            AZ_Assert(false, "preset should always exist");
            return nullptr;
        }

        //generate export file name
        QDir dir(exportDir.c_str());
        if (!dir.exists())
        {
            dir.mkpath(".");
        }
        AZStd::string fileName, outputPath;
        AzFramework::StringFunc::Path::GetFileName(imageFilePath.c_str(), fileName);
        fileName += ".dds";
        AzFramework::StringFunc::Path::Join(exportDir.c_str(), fileName.c_str(), outputPath, true, true);

        //if it need streaming
        bool isStreaming = BuilderSettingManager::Instance()->GetBuilderSetting(platformName)->m_enableStreaming;
        
        //create convert process
        ImageConvertProcess* process = new ImageConvertProcess(srcImage, textureSettings, *preset, false, isStreaming,
            canOverridePreset, outputPath, platformName);

        return process;
    }
    
    void ImageConvertProcess::CreateDiffuseCubemap()
    {
        //only need to convert if the diffuseGenPreset in cubemap setting is set
        if (m_presetSetting.m_cubemapSetting == nullptr || m_presetSetting.m_cubemapSetting->m_diffuseGenPreset.IsNull())
        {
            return;
        }

        //need to create another ImageConvertProcess 
        //prepare preset setting and texture setting
        const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(
            m_presetSetting.m_cubemapSetting->m_diffuseGenPreset, m_platformId);

        TextureSettings textureSettings = m_textureSetting;
        m_textureSetting.m_preset = m_presetSetting.m_cubemapSetting->m_diffuseGenPreset;

        if (preset == nullptr)
        {
            AZ_Error("Image Processing", false,"Couldn't find preset for diffuse cubemap generation");
            return;
        }

        //generate export file name. add "_diff" in the end of file name
        AZStd::string fileName, folderName, outProductPath;
        AzFramework::StringFunc::Path::GetFileName(m_outputPath.c_str(), fileName);
        AzFramework::StringFunc::Path::GetFullPath(m_outputPath.c_str(), folderName);
        fileName += "_diff.dds";
        AzFramework::StringFunc::Path::Join(folderName.c_str(), fileName.c_str(), outProductPath, true, true);

        //create convert process
        //we might be able to use current image result for the input to save some performance. But it's more safe to use input image
        bool canOverridePreset = false;
        ImageConvertProcess* process = new ImageConvertProcess(m_inputImage, textureSettings, *preset, 
            false, m_isStreaming, canOverridePreset, outProductPath, m_platformId);
        if (process)
        {
            process->ProcessAll();
            if (process->IsSucceed())
            {
                process->GetAppendOutputFilePaths(m_productFilepaths);
                m_diffCubemapImage = process->m_image->Get();
            }
            else
            {
                AZ_Error("Image Processing", false, "Convert diffuse cubemap failed");
            }
            delete process;
        }
        else
        {
            AZ_Error("Image Processing", false, "Create convert process for diffuse cubemap failed");
        }
    }

    bool ConvertImageFile(const AZStd::string& imageFilePath, const AZStd::string& exportDir, 
        AZStd::vector<AZStd::string>& outPaths, const PlatformName& platformName, AZ::SerializeContext* context)
    {
        bool result = false;
        ImageConvertProcess* process = CreateImageConvertProcess(imageFilePath, exportDir, platformName, context);
        if (process)
        {
            process->ProcessAll();
            result = process->IsSucceed();
            if (result)
            {
                process->GetAppendOutputFilePaths(outPaths);
            }
            delete process;
        }
        return result;
    }

    IImageObjectPtr MergeOutputImageForPreview(IImageObjectPtr image, IImageObjectPtr alphaImage)
    {
        if (!image)
        {
            return IImageObjectPtr();
        }

        ImageToProcess imageToProcess(image);
        imageToProcess.ConvertFormat(ePixelFormat_R8G8B8A8);
        IImageObjectPtr previewImage = imageToProcess.Get();

        // If there is separate Alpha image, combine it with output
        if (alphaImage)
        {
            // Create pixel operation function for rgb and alpha images
            IPixelOperationPtr imageOp = CreatePixelOperation(ePixelFormat_R8G8B8A8);
            IPixelOperationPtr alphaOp = CreatePixelOperation(ePixelFormat_A8);

            // Convert the alpha image to A8 first
            ImageToProcess imageToProcess2(alphaImage);
            imageToProcess2.ConvertFormat(ePixelFormat_A8);
            IImageObjectPtr previewImageAlpha = imageToProcess2.Get();

            const uint32 imageMips = previewImage->GetMipCount();
            const uint32 alphaMips = previewImageAlpha->GetMipCount();

            // Get count of bytes per pixel for both rgb and alpha images
            uint32 imagePixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(ePixelFormat_R8G8B8A8)->bitsPerBlock / 8;
            uint32 alphaPixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(ePixelFormat_A8)->bitsPerBlock / 8;

            AZ_Assert(imageMips <= alphaMips, "Mip level of alpha image is less than origin image!");

            // For each mip level, set the alpha value to the image
            for (uint32 mipLevel = 0; mipLevel < imageMips; ++mipLevel)
            {
                const uint32 pixelCount = previewImage->GetPixelCount(mipLevel);
                const uint32 alphaPixelCount = previewImageAlpha->GetPixelCount(mipLevel);

                AZ_Assert(pixelCount == alphaPixelCount, "Pixel count for image and alpha image at mip level %d is not equal!", mipLevel);

                uint8* imageBuf;
                uint32 pitch;
                previewImage->GetImagePointer(mipLevel, imageBuf, pitch);

                uint8* alphaBuf;
                uint32 alphaPitch;
                previewImageAlpha->GetImagePointer(mipLevel, alphaBuf, alphaPitch);

                float rAlpha, gAlpha, bAlpha, aAlpha, rImage, gImage, bImage, aImage;

                for (uint32 i = 0; i < pixelCount; ++i, imageBuf += imagePixelBytes, alphaBuf += alphaPixelBytes)
                {
                    alphaOp->GetRGBA(alphaBuf, rAlpha, gAlpha, bAlpha, aAlpha);
                    imageOp->GetRGBA(imageBuf, rImage, gImage, bImage, aImage);
                    imageOp->SetRGBA(imageBuf, rImage, gImage, bImage, aAlpha);
                }
            }
        }

        return previewImage;
    }

    // This function will convert compressed image to RGBA32.
    // Also if the image is in sRGB space will convert it to Linear space.
    IImageObjectPtr GetUncompressedLinearImage(IImageObjectPtr ddsImage)
    {
        if (ddsImage)
        {
            ImageToProcess processImage(ddsImage);
            if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(ddsImage->GetPixelFormat()))
            {
                processImage.ConvertFormat(ePixelFormat_R32G32B32A32F);
            }
            if (ddsImage->HasImageFlags(EIF_SRGBRead))
            {
                processImage.GammaToLinearRGBA32F(true);
            }
            return processImage.Get();
        }
        return nullptr;
    }

    float GetErrorBetweenImages(IImageObjectPtr inputImage1, IImageObjectPtr inputImage2)
    {
        // First make sure images are in uncompressed format and linear space
        // Convert them if necessary
        IImageObjectPtr image1 = GetUncompressedLinearImage(inputImage1);
        IImageObjectPtr image2 = GetUncompressedLinearImage(inputImage2);

        const float errorValue = FLT_MAX;

        if (!image1 || !image2)
        {
            AZ_Warning("Image Processing", false, "Invalid images passed into %s function", __FUNCTION__);
            return errorValue;
        }

        // Two images should share same size
        if (image1->GetWidth(0) != image2->GetWidth(0) || image1->GetHeight(0) != image2->GetHeight(0))
        {
            AZ_Warning("Image Processing", false, "%s function only can get error between two images with same size", __FUNCTION__);
            return errorValue;
        }

        //create pixel operation function
        IPixelOperationPtr pixelOp1 = CreatePixelOperation(image1->GetPixelFormat());
        IPixelOperationPtr pixelOp2 = CreatePixelOperation(image2->GetPixelFormat());

        //get count of bytes per pixel 
        AZ::u32 pixelBytes1 = CPixelFormats::GetInstance().GetPixelFormatInfo(image1->GetPixelFormat())->bitsPerBlock / 8;
        AZ::u32 pixelBytes2 = CPixelFormats::GetInstance().GetPixelFormatInfo(image2->GetPixelFormat())->bitsPerBlock / 8;

        float color1[4];
        float color2[4];
        AZ::u8* mem1;
        AZ::u8* mem2;
        uint32 pitch1, pitch2;

        float sumDeltaSqLinear  = 0;
        AZ::u32 sumPixelCount = 0;

        //only process the highest mip
        image1->GetImagePointer(0, mem1, pitch1);
        image2->GetImagePointer(0, mem2, pitch2);
            
        const uint32 pixelCount = image1->GetPixelCount(0);

        for (uint32 i = 0; i < pixelCount; ++i)
        {
            pixelOp1->GetRGBA(mem1, color1[0], color1[1], color1[2], color1[3]);
            pixelOp2->GetRGBA(mem2, color2[0], color2[1], color2[2], color2[3]);

            sumDeltaSqLinear += (color1[0] - color2[0]) * (color1[0] - color2[0])
                + (color1[1] - color2[1]) * (color1[1] - color2[1])
                + (color1[2] - color2[2]) * (color1[2] - color2[2]);

            mem1 += pixelBytes1;
            mem2 += pixelBytes2;
        }

        return sumDeltaSqLinear / pixelCount;
    }

    void GetBC1CompressionErrors(IImageObjectPtr originImage, float& errorLinear, float& errorSrgb, 
        ICompressor::CompressOption option)
    {
        errorLinear = 0;
        errorSrgb = 0;

        if (originImage->HasImageFlags(EIF_SRGBRead))
        {
            AZ_Assert(false, "The input origin image of %s function need be in linear color space", __FUNCTION__);
            return;
        }

        //compress and decompress in linear space
        ImageToProcess processLinear(originImage);
        processLinear.SetCompressOption(option);
        processLinear.ConvertFormat(ePixelFormat_BC1);
        processLinear.ConvertFormat(ePixelFormat_R32G32B32A32F);

        errorLinear = GetErrorBetweenImages(originImage, processLinear.Get());

        //compress and descompress in srgb space, then convert back to linear space to compare to original image
        ImageToProcess processSrgb(originImage);
        processSrgb.SetCompressOption(option);
        processSrgb.LinearToGamma();
        processSrgb.ConvertFormat(ePixelFormat_BC1);
        processSrgb.ConvertFormat(ePixelFormat_R32G32B32A32F);
        processSrgb.GammaToLinearRGBA32F(true);

        errorSrgb = GetErrorBetweenImages(originImage, processSrgb.Get());
    }
 
}// namespace ImageProcessing
