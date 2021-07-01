/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "EditorImageBuilderComponent.h"

#include <AssetBuilderSDK/SerializationDependencies.h>
#include <GradientSignal/ImageAsset.h>
#include <GradientSignal/ImageSettings.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <QImageReader>
#include <QDirIterator>
#include <GradientSignalSystemComponent.h>
#include <GradientSignal/GradientImageConversion.h>
#include <Atom/ImageProcessing/ImageObject.h>
#include <Atom/ImageProcessing/ImageProcessingBus.h>

namespace GradientSignal
{
    EditorImageBuilderPluginComponent::EditorImageBuilderPluginComponent()
    {
        // AZ Components should only initialize their members to null and empty in constructor
        // after construction, they may be deserialized from file.
    }

    EditorImageBuilderPluginComponent::~EditorImageBuilderPluginComponent()
    {
    }

    void EditorImageBuilderPluginComponent::Init()
    {
    }

    void EditorImageBuilderPluginComponent::Activate()
    {
        // Activate is where you'd perform registration with other objects and systems.
        // Since we want to register our builder, we do that here:
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Gradient Image Builder";
        builderDescriptor.m_version = 1;

        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.tif", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.tiff", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.png", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.bmp", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.jpg", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.jpeg", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.tga", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.gif", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.bt", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));

        builderDescriptor.m_busId = EditorImageBuilderWorker::GetUUID();
        builderDescriptor.m_createJobFunction = AZStd::bind(&EditorImageBuilderWorker::CreateJobs, &m_imageBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&EditorImageBuilderWorker::ProcessJob, &m_imageBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        m_imageBuilder.BusConnect(builderDescriptor.m_busId);

        EBUS_EVENT(AssetBuilderSDK::AssetBuilderBus, RegisterBuilderInformation, builderDescriptor);
    }

    void EditorImageBuilderPluginComponent::Deactivate()
    {
        m_imageBuilder.BusDisconnect();
    }

    void EditorImageBuilderPluginComponent::Reflect(AZ::ReflectContext* context)
    {
        ImageSettings::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorImageBuilderPluginComponent, AZ::Component>()->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }

    EditorImageBuilderWorker::EditorImageBuilderWorker()
    {
    }

    EditorImageBuilderWorker::~EditorImageBuilderWorker()
    {
    }

    void EditorImageBuilderWorker::ShutDown()
    {
        // it is important to note that this will be called on a different thread than your process job thread
        m_isShuttingDown = true;
    }

    // this happens early on in the file scanning pass
    // this function should consistently always create the same jobs, and should do no checking whether the job is up to date or not - just be consistent.
    void EditorImageBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, true, true);

        // Check file for _GSI suffix/pattern which assumes processing will occur whether or not settings are provided
        AZStd::string patternPath = fullPath;
        AZStd::to_upper(patternPath.begin(), patternPath.end());
        bool patternMatched = patternPath.rfind("_GSI.") != AZStd::string::npos;

        // Determine if a settings file has been provided
        AZStd::string settingsPath = fullPath + "." + s_gradientImageSettingsExtension;
        bool settingsExist = AZ::IO::SystemFile::Exists(settingsPath.data());

        // If the settings file is modified the image must be reprocessed
        AssetBuilderSDK::SourceFileDependency sourceFileDependency;
        sourceFileDependency.m_sourceFileDependencyPath = settingsPath;
        response.m_sourceFileDependencyList.push_back(sourceFileDependency);

        // If no settings file was provided then skip the file, unless the file name matches the convenience pattern
        if (!patternMatched && !settingsExist)
        {
            //do nothing if settings aren't provided
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        AZStd::unique_ptr<ImageSettings> settingsPtr;
        if (settingsExist)
        {
            settingsPtr.reset(AZ::Utils::LoadObjectFromFile<ImageSettings>(settingsPath));
        }

        // If the settings file didn't load then skip the file, unless the file name matches the convenience pattern
        if (!patternMatched && !settingsPtr)
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed to create gradient image conversion job for %s.\nFailed loading settings %s.\n", fullPath.data(), settingsPath.data());
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
            return;
        }

        // If settings loaded but processing is disabled then skip the file
        if (settingsPtr && !settingsPtr->m_shouldProcess)
        {
            //do nothing if settings disable processing
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        // Get the extension of the file
        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.data(), ext, false);
        AZStd::to_upper(ext.begin(), ext.end());

        // We process the same file for all platforms
        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = ext + " Compile (Gradient Image)";
            descriptor.SetPlatformIdentifier(info.m_identifier.data());
            descriptor.m_critical = false;
            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        return;
    }

    // later on, this function will be called for jobs that actually need doing.
    // the request will contain the CreateJobResponse you constructed earlier, including any keys and values you placed into the hash table
    void EditorImageBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        // Before we begin, let's make sure we are not meant to abort.
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
        if (jobCancelListener.IsCancelled())
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled gradient image conversion job for %s.\nCancellation requested.\n", request.m_fullPath.data());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled gradient image conversion job for %s.\nShutdown requested.\n", request.m_fullPath.data());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        // Do conversion and get exported file's path
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Performing gradient image conversion job for %s\n", request.m_fullPath.data());

        auto imageAsset = LoadImageFromPath(request.m_fullPath);

        if (!imageAsset)
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed gradient image conversion job for %s.\nFailed loading source image %s.\n", request.m_fullPath.data(), request.m_fullPath.data());
            return;
        }

        auto imageSettings = LoadImageSettingsFromPath(request.m_fullPath);

        if (!imageSettings)
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed gradient image conversion job for %s.\nFailed loading source image %s.\n", request.m_fullPath.data(), request.m_fullPath.data());
            return;
        }

        // ChannelMask is 8 bits, and the bits are assumed to be as follows: 0b0000ABGR
        imageAsset = ConvertImage(*imageAsset, *imageSettings);

        //generate export file name
        QDir dir(request.m_tempDirPath.data());
        if (!dir.exists())
        {
            dir.mkpath(".");
        }

        AZStd::string fileName;
        AZStd::string outputPath;
        AzFramework::StringFunc::Path::GetFileName(request.m_fullPath.data(), fileName);
        fileName += AZStd::string(".") + s_gradientImageExtension;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.data(), fileName.data(), outputPath, true, true);
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Output path for gradient image conversion: %s\n", outputPath.data());

        //save asset
        if (!AZ::Utils::SaveObjectToFile(outputPath, AZ::DataStream::ST_XML, imageAsset.get()))
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Failed gradient image conversion job for %s.\nFailed saving output file %s.\n", request.m_fullPath.data(), outputPath.data());
            return;
        }

        // Report the image-import result
        AssetBuilderSDK::JobProduct jobProduct;
        if(!AssetBuilderSDK::OutputObject(imageAsset.get(), outputPath, azrtti_typeid<ImageAsset>(), 2, jobProduct))
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Failed to output product dependencies.");
            return;
        }
        
        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Completed gradient image conversion job for %s.\nSucceeded saving output file %s.\n", request.m_fullPath.data(), outputPath.data());
    }

    AZ::Uuid EditorImageBuilderWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{7520DF20-16CA-4CF6-A6DB-D96759A09EE4}");
    }

    static AZStd::unique_ptr<ImageAsset> AtomLoadImageFromPath(const AZStd::string& fullPath)
    {
        ImageProcessingAtom::IImageObjectPtr imageObject;
        ImageProcessingAtom::ImageProcessingRequestBus::BroadcastResult(imageObject, &ImageProcessingAtom::ImageProcessingRequests::LoadImage,
            fullPath);

        if (!imageObject)
        {
            return {};
        }

        //create a new image asset
        auto imageAsset = AZStd::make_unique<ImageAsset>();

        if (!imageAsset)
        {
            return {};
        }

        imageAsset->m_imageWidth = imageObject->GetWidth(0);
        imageAsset->m_imageHeight = imageObject->GetHeight(0);
        imageAsset->m_imageFormat = imageObject->GetPixelFormat();

        AZ::u8* mem = nullptr;
        AZ::u32 pitch = 0;
        AZ::u32 mipBufferSize = imageObject->GetMipBufSize(0);
        imageObject->GetImagePointer(0, mem, pitch);

        imageAsset->m_imageData = { mem, mem + mipBufferSize };

        return imageAsset;
    }

    AZStd::unique_ptr<ImageAsset> EditorImageBuilderWorker::LoadImageFromPath(const AZStd::string& fullPath)
    {
        return AtomLoadImageFromPath(fullPath);
    }

    AZStd::unique_ptr<ImageSettings> EditorImageBuilderWorker::LoadImageSettingsFromPath(const AZStd::string& fullPath)
    {
        // Determine if a settings file has been provided
        AZStd::string settingsPath = fullPath + "." + s_gradientImageSettingsExtension;
        bool settingsExist = AZ::IO::SystemFile::Exists(settingsPath.data());

        if (settingsExist)
        {
            return AZStd::unique_ptr<ImageSettings>{AZ::Utils::LoadObjectFromFile<ImageSettings>(settingsPath)};
        }
        else
        {
            return AZStd::make_unique<ImageSettings>();
        }
    }

} // namespace GradientSignal
