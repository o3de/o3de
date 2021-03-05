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

#include <QRect>

#include <BuilderSettings/ImageProcessingDefines.h>
#include <BuilderSettings/PresetSettings.h>
#include <BuilderSettings/TextureSettings.h>
#include <ImageProcessing/ImageObject.h>
#include <Compressors/Compressor.h>

#include <AzCore/Jobs/Job.h>
#include <AzCore/std/string/string.h>

namespace ImageProcessing
{
    class IImageObject;
    class ImageToProcess;

    //Convert image file with its image export setting and save to specified folder. 
    //this function can be useful for a cancelable job 
    class ImageConvertProcess* CreateImageConvertProcess(const AZStd::string& imageFilePath,
        const AZStd::string& exportDir, const PlatformName& platformName, AZ::SerializeContext* context = nullptr);

    //Convert image file with its image export setting and save to specified folder. it will return when the whole conversion is done.
    //Could be used for command mode or test
    bool ConvertImageFile(const AZStd::string& imageFilePath, const AZStd::string& exportDir, AZStd::vector<AZStd::string>& outPaths, 
        const PlatformName& platformName = "", AZ::SerializeContext* context = nullptr);

    //image filter function
    void FilterImage(MipGenType genType, MipGenEvalType evalType, float blurH, float blurV, const IImageObjectPtr srcImg, int srcMip,
        IImageObjectPtr dstImg, int dstMip, QRect* srcRect, QRect* dstRect);

    //get compression error for an image converting to certain format
    void GetBC1CompressionErrors(IImageObjectPtr originImage, float& errorLinear, float& errorSrgb,
        ICompressor::CompressOption option);

    float GetErrorBetweenImages(IImageObjectPtr inputImage1, IImageObjectPtr inputImage2);

    //Combine image with alpha image if any and output as RGBA8
    IImageObjectPtr MergeOutputImageForPreview(IImageObjectPtr image, IImageObjectPtr alphaImage);

    //get output image size and mip count based on the texture setting and preset setting

    //other helper functions
    //Get desired output image size based on the texture settings
    void GetOutputExtent(AZ::u32 inputWidth, AZ::u32 inputHeight, AZ::u32& outWidth, AZ::u32& outHeight, AZ::u32& outReduce,
        const TextureSettings* textureSettings, const PresetSettings* presetSettings);
        
    class ImageConvertProcess
    {
    public:
        //constructor
        ImageConvertProcess(const IImageObjectPtr inputImage, const TextureSettings& textureSetting,
            const PresetSettings& presetSetting, bool isPreview, bool isStreaming, bool canOverridePreset,
            const AZStd::string& outputPath, const AZStd::string& platformId);
        ~ImageConvertProcess();

        //doing image conversion, this function need to be called repeatly until the process is done
        //it could used for a working thread which may need to cancel a process
        void UpdateProcess();

        //doing all conversion in one step. This function will call UpdateProcess in a while loop until it's done.
        void ProcessAll();

        //for multi-thread
        //get percentage of image convertion progress
        float GetProgress();
        bool IsFinished();
        bool IsSucceed();

        //get output images
        IImageObjectPtr GetOutputImage();
        IImageObjectPtr GetOutputAlphaImage();
        IImageObjectPtr GetOutputDiffCubemap();

        //get output file paths and append the paths to the outPaths vector. 
        void GetAppendOutputFilePaths(AZStd::vector<AZStd::string>& outPaths);

    private:
        enum ConvertStep
        {
            StepValidateInput = 0,
            StepGenerateColorChart,
            StepConvertToLinear,
            StepSwizzle,
            StepOverridePreset,
            StepCubemapLayout,
            StepPreNormalize,
            StepDiffCubemap,
            StepMipmap,
            StepGlossFromNormal,
            StepPostNormalize,
            StepCreateHighPass,
            StepConvertOutputColorSpace,
            StepAlphaImage,
            StepConvertPixelFormat,
            StepSaveToFile,
            StepAll
        };

        //input image and settings
        const IImageObjectPtr m_inputImage;
        TextureSettings m_textureSetting;
        PresetSettings m_presetSetting;
        bool m_canOverridePreset;
        bool m_isPreview;
        AZStd::string m_outputPath;
        AZStd::string m_platformId;

        //some global settings from builder setting
        bool m_isStreaming;

        //for alpha
        //to indicate the current alpha chanenl content
        EAlphaContent m_alphaContent;
        //An image object to hold alpha channel in a seperate image
        IImageObjectPtr m_alphaImage;

        //for cubemap
        //An image object to save output result of diffuse cubemap conversion
        IImageObjectPtr m_diffCubemapImage;

        //image for processing
        ImageToProcess *m_image;

        //progress
        uint32 m_progressStep;
        bool m_isFinished;
        bool m_isSucceed;

        //all the output products' paths
        AZStd::vector<AZStd::string> m_productFilepaths;

        //for get processing time
        AZStd::sys_time_t m_startTime;
        double m_processTime; //in seconds
        
    private:
        //validate the input image and settings
        bool ValidateInput();

        //mipmap generation
        bool FillMipmaps();

        //mipmap generation for cubemap
        bool FillCubemapMipmaps();

        //special case: create diffuse cubemap 
        void CreateDiffuseCubemap();

        //convert color space to linear with pixel format rgba32f
        bool ConvertToLinear();

        //convert to output color space before compression
        bool ConvertToOuputColorSpace();

        //create alpha image if it's needed
        void CreateAlphaImage();

        //pixel format convertion/compression
        bool ConvertPixelformat();
        
        //save output image to a file
        bool SaveOutput();

        //if it's converting for cubemap
        bool IsConvertToCubemap();
    };        

}// namespace ImageProcessing
