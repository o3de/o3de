/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QRect>

#include <BuilderSettings/ImageProcessingDefines.h>
#include <BuilderSettings/PresetSettings.h>
#include <BuilderSettings/TextureSettings.h>
#include <Atom/ImageProcessing/ImageObject.h>
#include <Compressors/Compressor.h>

#include <AzCore/Jobs/Job.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <ImageBuilderBaseType.h>

namespace ImageProcessingAtom
{
    class IImageObject;
    class ImageToProcess;

    //Convert image file with its image export setting and save to specified folder.
    //this function can be useful for a cancelable job
    class ImageConvertProcess* CreateImageConvertProcess(const AZStd::string& imageFilePath,
        const AZStd::string& exportDir, const PlatformName& platformName, AZStd::vector<AssetBuilderSDK::JobProduct>& jobProducts, AZ::SerializeContext* context = nullptr);

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

    //Converts the image to a RGBA8 format that can be displayed in a preview UI.
    IImageObjectPtr ConvertImageForPreview(IImageObjectPtr image);

    //get output image size and mip count based on the texture setting and preset setting

    //other helper functions
    //Get desired output image size based on the texture settings
    void GetOutputExtent(AZ::u32 inputWidth, AZ::u32 inputHeight, AZ::u32& outWidth, AZ::u32& outHeight, AZ::u32& outReduce,
        const TextureSettings* textureSettings, const PresetSettings* presetSettings);

    // The structure used to create a ImageConvertProcess
    struct ImageConvertProcessDescriptor
    {
        // The input image object
        IImageObjectPtr m_inputImage;
        TextureSettings m_textureSetting;
        PresetSettings m_presetSetting;
        // If the process is for preview convert result. Some steps will be optimized if it's true
        bool m_isPreview = false;
        // The target platform for the product asset
        AZStd::string m_platform;
        // Path to the original preset file, for debug output
        AZStd::string m_filePath;

        // The following parameters are required if it's not for preview mode

        // If the process is for preview convert result. Some steps will be optimized if it's true
        bool m_isStreaming = true;
        // The file name of the image which includes file extension. This is used to generate output file path.
        AZStd::string m_imageName;
        // The folder to save all output asset files. This is used to generate output file path.
        AZStd::string m_outputFolder;
        // Asset id of the source image file. This is mainly used to generate AssetId of ImageMipChainAsset which is referenced in StreamingPoolAsset
        AZ::Data::AssetId m_sourceAssetId;
        // List of output products for the job, appended to by the ImageConvertProcess
        AZStd::vector<AssetBuilderSDK::JobProduct>* m_jobProducts = nullptr;
    };

    /**
     * ImageConvertProcess is the class to handle the full convertion process to convert an input image object
     * to a new image object which is used in 3d renderer.
     */
    class ImageConvertProcess
    {
    public:
        //constructor
        ImageConvertProcess(AZStd::unique_ptr<ImageConvertProcessDescriptor>&& description);
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
        IImageObjectPtr GetOutputIBLSpecularCubemap();
        IImageObjectPtr GetOutputIBLDiffuseCubemap();

        // Get output JobProducts and append them to the outProducts vector.
        void GetAppendOutputProducts(AZStd::vector<AssetBuilderSDK::JobProduct>& outProducts);

        const ImageConvertProcessDescriptor* GetInputDesc() const;

    private:
        //input image and settings
        AZStd::shared_ptr<ImageConvertProcessDescriptor> m_input;

        //for alpha
        //to indicate the current alpha channel content
        EAlphaContent m_alphaContent;

        //output results of IBL cubemap generation, used in unit tests
        IImageObjectPtr m_iblSpecularCubemapImage;
        IImageObjectPtr m_iblDiffuseCubemapImage;

        //image for processing
        ImageToProcess* m_image;

        //progress
        uint32 m_progressStep;
        bool m_isFinished;
        bool m_isSucceed;

        //for get processing time
        AZStd::sys_time_t m_startTime;
        double m_processTime; //in seconds

        // All JobProducts from the process
        AZStd::vector<AssetBuilderSDK::JobProduct> m_jobProducts;
    private:
        //validate the input image and settings
        bool ValidateInput();

        //mipmap generation
        bool FillMipmaps();

        //mipmap generation for cubemap
        bool FillCubemapMipmaps();

        //IBL cubemap generation, this creates a separate ImageConvertProcess
        bool CreateIBLCubemap(PresetName preset, const char* fileNameSuffix, IImageObjectPtr& cubemapImage);

        //convert color space to linear with pixel format rgba32f
        bool ConvertToLinear();

        //convert to output color space before compression
        bool ConvertToOuputColorSpace();

        //pixel format convertion/compression
        bool ConvertPixelformat();

        //save output image to a file
        bool SaveOutput();

        //if it's converting for cubemap
        bool IsConvertToCubemap();

        //if the input image is a pre-convolved cubemap
        bool IsPreconvolvedCubemap();
    };
}// namespace ImageProcessingAtom
