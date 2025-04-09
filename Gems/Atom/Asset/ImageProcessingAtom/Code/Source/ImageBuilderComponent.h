/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Asset/AssetManager.h>
#include <Atom/ImageProcessing/ImageProcessingBus.h>

namespace ImageProcessingAtom
{
    //! Builder to process images
    class ImageBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(ImageBuilderWorker, "{7F1FA09D-77F3-4118-A7D5-4906BED59C19}");

        ImageBuilderWorker() = default;
        ~ImageBuilderWorker() = default;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override; // if you get this you must fail all existing jobs and return.
        //////////////////////////////////////////////////////////////////////////

    private:
        bool m_isShuttingDown = false;
    };

    //! BuilderPluginComponent is to handle the lifecycle of ImageBuilder module.
    class BuilderPluginComponent
        : public AZ::Component
        , protected ImageProcessingRequestBus::Handler
        , protected ImageBuilderRequestBus::Handler
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{A227F803-D2E4-406E-93EC-121EF45A64A1}")
        static void Reflect(AZ::ReflectContext* context);

        BuilderPluginComponent(); // avoid initialization here.
        ~BuilderPluginComponent() override; // free memory an uninitialize yourself.

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override; // create objects, allocate memory and initialize yourself without reaching out to the outside world
        void Activate() override; // reach out to the outside world and connect up to what you need to, register things, etc.
        void Deactivate() override; // unregister things, disconnect from the outside world

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AtomImageProcessingRequestBus interface implementation
        IImageObjectPtr LoadImage(const AZStd::string& filePath) override;
        IImageObjectPtr LoadImagePreview(const AZStd::string& filePath) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ImageBuilderRequestBus interface implementation
        IImageObjectPtr CreateImage(
            AZ::u32 width,
            AZ::u32 height,
            AZ::u32 maxMipCount,
            EPixelFormat pixelFormat) override;
        AZStd::vector<AssetBuilderSDK::JobProduct> ConvertImageObject(
            IImageObjectPtr imageObject,
            const AZStd::string& presetName,
            const AZStd::string& platformName,
            const AZStd::string& outputDir,
            const AZ::Data::AssetId& sourceAssetId,
            const AZStd::string& sourceAssetName) override;
        IImageObjectPtr ConvertImageObjectInMemory(
            IImageObjectPtr imageObject,
            const AZStd::string& presetName,
            const AZStd::string& platformName,
            const AZ::Data::AssetId& sourceAssetId,
            const AZStd::string& sourceAssetName) override;
        bool DoesSupportPlatform(const AZStd::string& platformId) override;
        bool IsPresetFormatSquarePow2(const AZStd::string& presetName, const AZStd::string& platformName) override;
        FileMask GetFileMask(AZStd::string_view imageFilePath) override;
        AZStd::vector<AZStd::string> GetFileMasksForPreset(const PresetName& presetName) override;
        AZStd::vector<PresetName> GetPresetsForFileMask(const FileMask& fileMask) override;
        PresetName GetDefaultPreset() override;
        PresetName GetDefaultAlphaPreset() override;
        bool IsValidPreset(PresetName presetName) override;

        bool IsExtensionSupported(const char* extension) override;

        ////////////////////////////////////////////////////////////////////////

    private:
        BuilderPluginComponent(const BuilderPluginComponent&) = delete;

        ImageBuilderWorker m_imageBuilder;

        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
    };
}// namespace ImageProcessingAtom

