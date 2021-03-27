/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Outcome/Outcome.h>
#include <ImageProcessing/ImageProcessingBus.h>

namespace ImageProcessing
{
    //! Builder to process images 
    class ImageBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(ImageBuilderWorker, "{525422DE-05B3-4095-966F-90CD7657A7E1}");

        ImageBuilderWorker() = default;
        ~ImageBuilderWorker() = default;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override; // if you get this you must fail all existing jobs and return.
        //////////////////////////////////////////////////////////////////////////

        //! Populates the jobProduct vector with all the entries including their product dependencies 
        AZ::Outcome<void, AZStd::string> PopulateProducts(const AssetBuilderSDK::ProcessJobRequest& request, const AZStd::vector<AZStd::string>& productFilepaths, AZStd::vector<AssetBuilderSDK::JobProduct>& jobProducts);

    private:
        bool m_isShuttingDown = false;
    };

    //! BuilderPluginComponent is to handle the lifecycle of ImageBuilder module.
    class BuilderPluginComponent
        : public AZ::Component
        , protected ImageProcessingRequestBus::Handler
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{2F12E1BE-D8F6-47A4-AC3E-6C5527C55840}")
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
        // ImageProcessingRequestBus interface implementation
        IImageObjectPtr LoadImage(const AZStd::string& filePath) override;
        IImageObjectPtr LoadImagePreview(const AZStd::string& filePath) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        ImageBuilderWorker m_imageBuilder;
    };
}// namespace ImageProcessing
