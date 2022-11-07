/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RHI.Reflect/StreamingImagePoolDescriptor.h>

#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>

#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

#include <ResourcePool/ResourcePoolBuilder.h>

#include <Tests.Builders/BuilderTestFixture.h>

namespace UnitTest
{
    using namespace AZ;    

    class ResourcePoolBuilderTests
        : public BuilderTestFixture
    {
    protected:
        AZStd::unique_ptr<RPI::StreamingImagePoolAssetHandler> m_streamingImagePoolAssetHandler;
        AZStd::unique_ptr<RPI::ResourcePoolAssetHandler> m_resourcePoolAssetHandler;

        void SetUp() override
        {
            BuilderTestFixture::SetUp();

            m_streamingImagePoolAssetHandler = AZStd::make_unique<RPI::StreamingImagePoolAssetHandler>();
            m_resourcePoolAssetHandler = AZStd::make_unique<RPI::ResourcePoolAssetHandler>();
            m_streamingImagePoolAssetHandler->m_serializeContext = m_context.get();
            m_resourcePoolAssetHandler->m_serializeContext = m_context.get();
            m_streamingImagePoolAssetHandler->Register();
            m_resourcePoolAssetHandler->Register();
        }

        void TearDown() override
        {
            m_streamingImagePoolAssetHandler->Unregister();
            m_resourcePoolAssetHandler->Unregister();
            m_streamingImagePoolAssetHandler.reset();
            m_resourcePoolAssetHandler.reset();

            BuilderTestFixture::TearDown();
        }        
    };
        
    TEST_F(ResourcePoolBuilderTests, ProcessJob_OutputBufferPool)
    {
        RPI::ResourcePoolBuilder builder;
        AssetBuilderSDK::ProcessJobRequest request;
        AssetBuilderSDK::ProcessJobResponse response;

        AZ::Test::ScopedAutoTempDirectory productDir;
        AZ::Test::ScopedAutoTempDirectory sourceDir;
        const char* testAssetName = "TestBufferPool.resourcepool";
        AZ::IO::Path sourceFilePath = sourceDir.Resolve(testAssetName);

        // Initial job request
        request.m_fullPath = sourceFilePath.Native();
        request.m_tempDirPath = productDir.GetDirectory();

        RPI::ResourcePoolSourceData sourceData;
        sourceData.m_poolName = "DefaultIndexBufferPool";
        sourceData.m_poolType = RPI::ResourcePoolAssetType::BufferPool;
        sourceData.m_budgetInBytes = 25165824;
        sourceData.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
        sourceData.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
        sourceData.m_bufferPoolBindFlags = RHI::BufferBindFlags::InputAssembly;

        Outcome<void, AZStd::string> saveResult = AZ::JsonSerializationUtils::SaveObjectToFile<RPI::ResourcePoolSourceData>(&sourceData,
            sourceFilePath.Native());
        
        // Process
        builder.ProcessJob(request, response);

        // verify job output
        EXPECT_TRUE(response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success);
        EXPECT_TRUE(response.m_outputProducts.size() == 1);
        EXPECT_TRUE(response.m_outputProducts[0].m_dependencies.size() == 0);

        // verify output file and verify loaded asset
        auto outAsset = Utils::LoadObjectFromFile<RPI::ResourcePoolAsset>(response.m_outputProducts[0].m_productFileName, m_context.get());
        auto poolDescriptor = outAsset->GetPoolDescriptor();
        EXPECT_TRUE(sourceData.m_poolName == outAsset->GetPoolName());
        EXPECT_TRUE(azrtti_typeid <RHI::BufferPoolDescriptor>() == azrtti_typeid(poolDescriptor.get()));
        auto bufferPoolDesc = azrtti_cast<RHI::BufferPoolDescriptor*>(poolDescriptor.get());
        EXPECT_TRUE(sourceData.m_budgetInBytes == bufferPoolDesc->m_budgetInBytes);
        EXPECT_TRUE(sourceData.m_heapMemoryLevel == bufferPoolDesc->m_heapMemoryLevel);
        EXPECT_TRUE(sourceData.m_hostMemoryAccess == bufferPoolDesc->m_hostMemoryAccess);
        EXPECT_TRUE(sourceData.m_bufferPoolBindFlags == bufferPoolDesc->m_bindFlags);
        delete outAsset;
    }
    
    TEST_F(ResourcePoolBuilderTests, ProcessJob_OutputImagePool)
    {
        RPI::ResourcePoolBuilder builder;
        AssetBuilderSDK::ProcessJobRequest request;
        AssetBuilderSDK::ProcessJobResponse response;
        
        AZ::Test::ScopedAutoTempDirectory productDir;
        AZ::Test::ScopedAutoTempDirectory sourceDir;
        const char* testAssetName = "TestImagePool.resourcepool";
        AZ::IO::Path sourceFilePath = sourceDir.Resolve(testAssetName);

        // Initial job request
        request.m_fullPath = sourceFilePath.Native();
        request.m_tempDirPath = productDir.GetDirectory();

        RPI::ResourcePoolSourceData sourceData;
        sourceData.m_poolName = "DefaultImagePool";
        sourceData.m_poolType = RPI::ResourcePoolAssetType::ImagePool;
        sourceData.m_budgetInBytes = 25165824;
        sourceData.m_imagePoolBindFlags = RHI::ImageBindFlags::Color;

        Outcome<void, AZStd::string> saveResult = AZ::JsonSerializationUtils::SaveObjectToFile<RPI::ResourcePoolSourceData>(&sourceData,
            sourceFilePath.Native());

        // Process
        builder.ProcessJob(request, response);

        // verify job output
        EXPECT_TRUE(response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success);
        EXPECT_TRUE(response.m_outputProducts.size() == 1);
        EXPECT_TRUE(response.m_outputProducts[0].m_dependencies.size() == 0);

        // verify output file and verify loaded asset
        auto outAsset = Utils::LoadObjectFromFile<RPI::ResourcePoolAsset>(response.m_outputProducts[0].m_productFileName, m_context.get());
        auto poolDescriptor = outAsset->GetPoolDescriptor();
        EXPECT_TRUE(sourceData.m_poolName == outAsset->GetPoolName());
        EXPECT_TRUE(azrtti_typeid<RHI::ImagePoolDescriptor>() == azrtti_typeid(poolDescriptor.get()));
        auto imagePoolDesc = azrtti_cast<RHI::ImagePoolDescriptor*>(poolDescriptor.get());
        EXPECT_TRUE(sourceData.m_budgetInBytes == imagePoolDesc->m_budgetInBytes);
        EXPECT_TRUE(sourceData.m_imagePoolBindFlags == imagePoolDesc->m_bindFlags);
        delete outAsset;
    }

    TEST_F(ResourcePoolBuilderTests, ProcessJob_OutputStreamingImagePool)
    {
        RPI::ResourcePoolBuilder builder;
        AssetBuilderSDK::ProcessJobRequest request;
        AssetBuilderSDK::ProcessJobResponse response;
        
        AZ::Test::ScopedAutoTempDirectory productDir;
        AZ::Test::ScopedAutoTempDirectory sourceDir;
        const char* testAssetName = "TestStreamingImagePool.resourcepool";
        AZ::IO::Path sourceFilePath = sourceDir.Resolve(testAssetName);

        // Initial job request
        request.m_fullPath = sourceFilePath.Native();
        request.m_tempDirPath = productDir.GetDirectory();

        RPI::ResourcePoolSourceData sourceData;
        sourceData.m_poolName = "DefaultStreamingImagePool";
        sourceData.m_poolType = RPI::ResourcePoolAssetType::StreamingImagePool;
        sourceData.m_budgetInBytes = 2147483648;

        Outcome<void, AZStd::string> saveResult = AZ::JsonSerializationUtils::SaveObjectToFile<RPI::ResourcePoolSourceData>(&sourceData,
            sourceFilePath.Native());

        // Process
        builder.ProcessJob(request, response);

        // verify job output
        EXPECT_TRUE(response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success);
        EXPECT_TRUE(response.m_outputProducts.size() == 1);
        EXPECT_TRUE(response.m_outputProducts[0].m_dependencies.size() == 0);

        // verify output file and verify loaded asset
        Utils::FilterDescriptor filter; // disable loading the controller asset inside
        filter.m_assetCB = AZ::Data::AssetFilterNoAssetLoading;

        auto outAsset = Utils::LoadObjectFromFile<RPI::StreamingImagePoolAsset>(response.m_outputProducts[0].m_productFileName, m_context.get(), filter);
        auto poolDescriptor = outAsset->GetPoolDescriptor();
        EXPECT_TRUE(azrtti_typeid <RHI::StreamingImagePoolDescriptor>() == azrtti_typeid(poolDescriptor));
        EXPECT_TRUE(sourceData.m_budgetInBytes == poolDescriptor.m_budgetInBytes);
        delete outAsset;
    }

  
} // namespace UnitTests
