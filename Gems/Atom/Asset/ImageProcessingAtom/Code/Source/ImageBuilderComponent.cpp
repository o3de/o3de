/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageBuilderComponent.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Debug/Trace.h>
#include <BuilderSettings/BuilderSettingManager.h>
#include <BuilderSettings/CubemapSettings.h>
#include <ImageLoader/ImageLoaders.h>
#include <Processing/ImageAssetProducer.h>
#include <Processing/ImageConvert.h>
#include <Processing/ImageToProcess.h>
#include <Processing/PixelFormatInfo.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <QFile>

#include <AzQtComponents/Utilities/QtPluginPaths.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>

namespace ImageProcessingAtom
{
    BuilderPluginComponent::BuilderPluginComponent()
    {
        // AZ Components should only initialize their members to null and empty in constructor
        // after construction, they may be deserialized from file.
    }

    BuilderPluginComponent::~BuilderPluginComponent()
    {
    }

    void BuilderPluginComponent::Init()
    {
    }

    void BuilderPluginComponent::Activate()
    {
        //load qt plugins for some image file formats support
        AzQtComponents::PrepareQtPaths();

        // create and initialize BuilderSettingManager once since it's will be used for image conversion
        BuilderSettingManager::CreateInstance();

        auto outcome = BuilderSettingManager::Instance()->LoadConfig();
        AZ_Error("Image Processing", outcome.IsSuccess(), "Failed to load Atom image builder settings.");

        if (!outcome.IsSuccess())
        {
            return;
        }

        // Activate is where you'd perform registration with other objects and systems.
        // Since we want to register our builder, we do that here:
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Atom Image Builder";

        for (int i = 0; i < s_TotalSupportedImageExtensions; i++)
        {
            builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern(s_SupportedImageExtensions[i], AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        }

        builderDescriptor.m_busId = azrtti_typeid<ImageBuilderWorker>();
        builderDescriptor.m_createJobFunction = AZStd::bind(&ImageBuilderWorker::CreateJobs, &m_imageBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&ImageBuilderWorker::ProcessJob, &m_imageBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_version = 35;   // Added MipmapChain and StreamingImage allocator
        builderDescriptor.m_analysisFingerprint = ImageProcessingAtom::BuilderSettingManager::Instance()->GetAnalysisFingerprint();
        m_imageBuilder.BusConnect(builderDescriptor.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);

        m_assetHandlers.emplace_back(AZ::RPI::MakeAssetHandler<AZ::RPI::ImageMipChainAssetHandler>());
        m_assetHandlers.emplace_back(AZ::RPI::MakeAssetHandler<AZ::RPI::StreamingImageAssetHandler>());

        ImageProcessingRequestBus::Handler::BusConnect();
        ImageBuilderRequestBus::Handler::BusConnect();
    }

    void BuilderPluginComponent::Deactivate()
    {
        ImageProcessingRequestBus::Handler::BusDisconnect();
        ImageBuilderRequestBus::Handler::BusDisconnect();
        m_imageBuilder.BusDisconnect();
        BuilderSettingManager::DestroyInstance();
        CPixelFormats::DestroyInstance();
    }

    void BuilderPluginComponent::Reflect(AZ::ReflectContext* context)
    {
        // components also get Reflect called automatically
        // this is your opportunity to perform static reflection or type registration of any types you want the serializer to know about
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BuilderPluginComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }

        BuilderSettingManager::Reflect(context);
        BuilderSettings::Reflect(context);
        MultiplatformPresetSettings::Reflect(context);
        PresetSettings::Reflect(context);
        CubemapSettings::Reflect(context);
        MipmapSettings::Reflect(context);
        TextureSettings::Reflect(context);
    }

    void BuilderPluginComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ImagerBuilderPluginService"));
    }

    void BuilderPluginComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ImagerBuilderPluginService"));
    }

    IImageObjectPtr BuilderPluginComponent::LoadImage(const AZStd::string& filePath)
    {
        return IImageObjectPtr(LoadImageFromFile(filePath));
    }

    IImageObjectPtr BuilderPluginComponent::LoadImagePreview(const AZStd::string& filePath)
    {
        IImageObjectPtr image(LoadImageFromFile(filePath));
        if (image)
        {
            ImageToProcess imageToProcess(image);
            imageToProcess.ConvertFormat(ePixelFormat_R8G8B8A8);
            return imageToProcess.Get();
        }
        return image;
    }

    IImageObjectPtr BuilderPluginComponent::CreateImage(
        AZ::u32 width,
        AZ::u32 height,
        AZ::u32 maxMipCount,
        EPixelFormat pixelFormat)
    {
        IImageObjectPtr image(IImageObject::CreateImage(width, height, maxMipCount, pixelFormat));
        return image;
    }

    AZStd::vector<AssetBuilderSDK::JobProduct> BuilderPluginComponent::ConvertImageObject(
        IImageObjectPtr imageObject,
        const AZStd::string& presetName,
        const AZStd::string& platformName,
        const AZStd::string& outputDir,
        const AZ::Data::AssetId& sourceAssetId,
        const AZStd::string& sourceAssetName)
    {
        AZStd::vector<AssetBuilderSDK::JobProduct> outProducts;

        AZStd::string_view presetFilePath;
        const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(PresetName(presetName), platformName, &presetFilePath);
        if (preset == nullptr)
        {
            AZ_Assert(false, "Cannot find preset with name %s.", presetName.c_str());
            return outProducts;
        }

        AZStd::unique_ptr<ImageConvertProcessDescriptor> desc = AZStd::make_unique<ImageConvertProcessDescriptor>();
        TextureSettings& textureSettings = desc->m_textureSetting;
        textureSettings.m_preset = preset->m_name;
        desc->m_inputImage = imageObject;
        desc->m_presetSetting = *preset;
        desc->m_isPreview = false;
        desc->m_platform = platformName;
        desc->m_filePath = presetFilePath;
        desc->m_isStreaming = BuilderSettingManager::Instance()->GetBuilderSetting(platformName)->m_enableStreaming;
        desc->m_imageName = sourceAssetName;
        desc->m_outputFolder = outputDir;
        desc->m_sourceAssetId = sourceAssetId;
        
        // Create an image convert process
        ImageConvertProcess process(AZStd::move(desc));
        process.ProcessAll();
        bool result = process.IsSucceed();
        if (result)
        {
            process.GetAppendOutputProducts(outProducts);
        }

        return outProducts;
    }

    IImageObjectPtr BuilderPluginComponent::ConvertImageObjectInMemory(
        IImageObjectPtr imageObject,
        const AZStd::string& presetName,
        const AZStd::string& platformName,
        const AZ::Data::AssetId& sourceAssetId,
        const AZStd::string& sourceAssetName)
    {
        AZStd::vector<AssetBuilderSDK::JobProduct> outProducts;

        AZStd::string_view presetFilePath;
        const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(PresetName(presetName), platformName, &presetFilePath);
        if (preset == nullptr)
        {
            AZ_Assert(false, "Cannot find preset with name %s.", presetName.c_str());
            return nullptr;
        }

        AZStd::unique_ptr<ImageConvertProcessDescriptor> desc = AZStd::make_unique<ImageConvertProcessDescriptor>();
        TextureSettings& textureSettings = desc->m_textureSetting;
        textureSettings.m_preset = preset->m_name;
        desc->m_inputImage = imageObject;
        desc->m_presetSetting = *preset;
        desc->m_isPreview = false;
        desc->m_platform = platformName;
        desc->m_filePath = presetFilePath;
        desc->m_isStreaming = BuilderSettingManager::Instance()->GetBuilderSetting(platformName)->m_enableStreaming;
        desc->m_imageName = sourceAssetName;
        desc->m_shouldSaveFile = false;
        desc->m_sourceAssetId = sourceAssetId;
        
        // Create an image convert process
        ImageConvertProcess process(AZStd::move(desc));
        process.ProcessAll();
        bool result = process.IsSucceed();
        if (result)
        {
            return process.GetOutputImage();
        }
        else
        {
            return nullptr;
        }
    }

    bool BuilderPluginComponent::DoesSupportPlatform(const AZStd::string& platformId)
    {
        return ImageProcessingAtom::BuilderSettingManager::Instance()->DoesSupportPlatform(platformId);
    }

    bool BuilderPluginComponent::IsPresetFormatSquarePow2(const AZStd::string& presetName, const AZStd::string& platformName)
    {
        AZStd::string_view filePath;
        const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(PresetName(presetName), platformName, &filePath);
        if (preset == nullptr)
        {
            AZ_Assert(false, "Cannot find preset with name %s.", presetName.c_str());
            return false;
        }

        const PixelFormatInfo* info = CPixelFormats::GetInstance().GetPixelFormatInfo(preset->m_pixelFormat);
        return info->bSquarePow2;
    }

    FileMask BuilderPluginComponent::GetFileMask(AZStd::string_view imageFilePath)
    {
        return ImageProcessingAtom::BuilderSettingManager::Instance()->GetFileMask(imageFilePath);
    }

    AZStd::vector<AZStd::string> BuilderPluginComponent::GetFileMasksForPreset(const PresetName& presetName)
    {
        return ImageProcessingAtom::BuilderSettingManager::Instance()->GetFileMasksForPreset(presetName);
    }

    AZStd::vector<PresetName> BuilderPluginComponent::GetPresetsForFileMask(const FileMask& fileMask)
    {
        return ImageProcessingAtom::BuilderSettingManager::Instance()->GetPresetsForFileMask(fileMask);
    }

    PresetName BuilderPluginComponent::GetDefaultPreset()
    {
        return ImageProcessingAtom::BuilderSettingManager::Instance()->GetDefaultPreset();
    }

    PresetName BuilderPluginComponent::GetDefaultAlphaPreset()
    {
        return ImageProcessingAtom::BuilderSettingManager::Instance()->GetDefaultAlphaPreset();
    }

    bool BuilderPluginComponent::IsValidPreset(PresetName presetName)
    {
        return ImageProcessingAtom::BuilderSettingManager::Instance()->IsValidPreset(presetName);
    }

    bool BuilderPluginComponent::IsExtensionSupported(const char* extension)
    {
        return ImageProcessingAtom::IsExtensionSupported(extension);
    }

    void ImageBuilderWorker::ShutDown()
    {
        // it is important to note that this will be called on a different thread than your process job thread
        m_isShuttingDown = true;
    }

    PresetName GetImagePreset(const AZStd::string& imageFileFullPath)
    {
        // first let preset from asset info
        TextureSettings textureSettings;
        AZStd::string settingFilePath = imageFileFullPath + TextureSettings::ExtensionName;
        TextureSettings::LoadTextureSetting(settingFilePath, textureSettings);

        if (!textureSettings.m_preset.IsEmpty())
        {
            return textureSettings.m_preset;
        }

        return BuilderSettingManager::Instance()->GetSuggestedPreset(imageFileFullPath);
    }

    void HandlePresetDependency(PresetName presetName, AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceDependencyList)
    {
        // Reload preset if it was changed
        ImageProcessingAtom::BuilderSettingManager::Instance()->ReloadPreset(presetName);
        
        auto presetSettings = BuilderSettingManager::Instance()->GetPreset(presetName, /*default platform*/"");

        AssetBuilderSDK::SourceFileDependency sourceFileDependency;
        sourceFileDependency.m_sourceDependencyType = AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Absolute;

        // Need to watch any possibe preset paths 
        AZStd::vector<AZStd::string> possiblePresetPaths = BuilderSettingManager::Instance()->GetPossiblePresetPaths(presetName);
        for (const auto& path:possiblePresetPaths)
        {
            sourceFileDependency.m_sourceFileDependencyPath = path;
            sourceDependencyList.push_back(sourceFileDependency);
        }

        if (presetSettings)
        {
            // handle special case here
            // Cubemap setting may reference some other presets
            if (presetSettings->m_cubemapSetting)
            {
                if (presetSettings->m_cubemapSetting->m_generateIBLDiffuse && !presetSettings->m_cubemapSetting->m_iblDiffusePreset.IsEmpty())
                {
                    HandlePresetDependency(presetSettings->m_cubemapSetting->m_iblDiffusePreset, sourceDependencyList);
                }
            
                if (presetSettings->m_cubemapSetting->m_generateIBLSpecular && !presetSettings->m_cubemapSetting->m_iblSpecularPreset.IsEmpty())
                {
                    HandlePresetDependency(presetSettings->m_cubemapSetting->m_iblSpecularPreset, sourceDependencyList);
                }
            }
        }
    }

    void ReloadPresetIfNeeded(PresetName presetName)
    {
        // Reload preset if it was changed
        ImageProcessingAtom::BuilderSettingManager::Instance()->ReloadPreset(presetName);
        
        auto presetSettings = BuilderSettingManager::Instance()->GetPreset(presetName, /*default platform*/"");

        if (presetSettings)
        {
            // handle special case here
            // Cubemap setting may reference some other presets
            if (presetSettings->m_cubemapSetting)
            {
                if (presetSettings->m_cubemapSetting->m_generateIBLDiffuse && !presetSettings->m_cubemapSetting->m_iblDiffusePreset.IsEmpty())
                {
                    ImageProcessingAtom::BuilderSettingManager::Instance()->ReloadPreset(presetSettings->m_cubemapSetting->m_iblDiffusePreset);
                }
            
                if (presetSettings->m_cubemapSetting->m_generateIBLSpecular && !presetSettings->m_cubemapSetting->m_iblSpecularPreset.IsEmpty())
                {
                    ImageProcessingAtom::BuilderSettingManager::Instance()->ReloadPreset(presetSettings->m_cubemapSetting->m_iblSpecularPreset);
                }
            }
        }
    }

    // this happens early on in the file scanning pass
    // this function should consistently always create the same jobs, and should do no checking whether the job is up to date or not - just be consistent.
    void ImageBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        // Full path of the image file        
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, true, true);

        // Get the extension of the file
        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);
        AZStd::to_upper(ext.begin(), ext.end());

        // We process the same file for all platforms
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            if (ImageProcessingAtom::BuilderSettingManager::Instance()->DoesSupportPlatform(platformInfo.m_identifier))
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Image Compile: " + ext;
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
                descriptor.m_critical = false;
                descriptor.m_additionalFingerprintInfo = "";
                response.m_createJobOutputs.push_back(descriptor);
            }
        }

        // add source dependency for .assetinfo file
        AssetBuilderSDK::SourceFileDependency sourceFileDependency;
        sourceFileDependency.m_sourceDependencyType = AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Absolute;
        sourceFileDependency.m_sourceFileDependencyPath = fullPath + TextureSettings::ExtensionName;
        response.m_sourceFileDependencyList.push_back(sourceFileDependency);

        // add source dependencies for .preset files
        // Get the preset for this file    
        auto presetName = GetImagePreset(fullPath.c_str());
        HandlePresetDependency(presetName, response.m_sourceFileDependencyList);

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        return;
    }

    // later on, this function will be called for jobs that actually need doing.
    // the request will contain the CreateJobResponse you constructed earlier, including any keys and values you placed into the hash table
    void ImageBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        // Before we begin, let's make sure we are not meant to abort.
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        AZStd::vector<AZStd::string> productFilepaths;
        bool imageProcessingSuccessful = false;
        bool needConversion = true;

        // Do conversion and get exported file's path
        if (needConversion)
        {

            // Handles preset changes
            auto presetName = GetImagePreset(request.m_fullPath);
            ReloadPresetIfNeeded(presetName);

            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Performing image conversion: %s\n", request.m_fullPath.c_str());
            ImageConvertProcess* process = CreateImageConvertProcess(request.m_fullPath, request.m_tempDirPath,
                request.m_jobDescription.GetPlatformIdentifier(), response.m_outputProducts);

            if (process != nullptr)
            {
                //the process can be stopped if the job is cancelled or the worker is shutting down
                while (!process->IsFinished() && !m_isShuttingDown && !jobCancelListener.IsCancelled())
                {
                    process->UpdateProcess();
                }

                //get process result
                imageProcessingSuccessful = process->IsSucceed();
                if (imageProcessingSuccessful)
                {
                    process->GetAppendOutputProducts(response.m_outputProducts);
                }

                delete process;
            }
            else
            {
                imageProcessingSuccessful = false;
            }
        }

        if (imageProcessingSuccessful)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }
        else
        {
            if (m_isShuttingDown)
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            }
            else if (jobCancelListener.IsCancelled())
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled was requested for job %s.\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            }
            else
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Unexpected error during processing job %s.\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            }
        }
    }
} // namespace ImageProcessingAtom
