/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RHI.Reflect/ReflectSystemComponent.h>

#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>
#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Pass/PassBuilder.h>

#include <Tests.Builders/BuilderTestFixture.h>

namespace UnitTest
{
    using namespace AZ;
    
    class PassBuilderTests
        : public BuilderTestFixture
    {
    protected:

        RPI::PassAssetHandler* m_assetHandler;

        void SetUp() override
        {
            BuilderTestFixture::SetUp();

            m_assetHandler = new RPI::PassAssetHandler();
            m_assetHandler->m_serializeContext = m_context.get();
            m_assetHandler->Register();
        }

        void TearDown() override
        {
            m_assetHandler->Unregister();
            delete m_assetHandler;
            BuilderTestFixture::TearDown();
        }

        template<typename T>
        void SaveAssetToFile(T& data, const char* saveFileName)
        {
            Utils::SaveObjectToFile(saveFileName, AZ::DataStream::ST_XML, &data, T::TYPEINFO_Uuid(), m_context.get());
        }

        Data::Asset<Data::AssetData> LoadAssetFromFile(const char* assetFile)
        {
            Data::Asset<Data::AssetData> outAsset(m_assetHandler->CreateAsset(Data::AssetId(Uuid::CreateRandom(), 1), Data::AssetType()),
                Data::AssetLoadBehavior::PreLoad);
            AZ::u64 fileLength = 0;
            AZ::IO::FileIOBase::GetInstance()->Size(assetFile, fileLength);
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream = AZStd::make_shared<AZ::Data::AssetDataStream>();
            stream->Open(assetFile, 0, fileLength);
            stream->BlockUntilLoadComplete();
            m_assetHandler->LoadAssetData(outAsset, stream, {});
            stream->Close();

            // Force a file streamer flush to ensure that file handles don't remain used
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            AZStd::binary_semaphore wait;
            AZ::IO::FileRequestPtr flushRequest = streamer->FlushCaches();
            streamer->SetRequestCompleteCallback(flushRequest, [&wait]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                {
                    wait.release();
                });
            streamer->QueueRequest(flushRequest);
            wait.acquire();

            return outAsset;
        }

        void SetPassTemplateForTestingOnly(RPI::PassAsset& passAsset, RPI::PassTemplate& passTemplate)
        {
            passAsset.SetPassTemplateForTestingOnly(passTemplate);
        }
    };
    
    TEST_F(PassBuilderTests, ProcessJob)
    {
        const char* testAssetName = "PassTestAsset.pass";
        AZ::Test::ScopedAutoTempDirectory productDir;
        AZ::Test::ScopedAutoTempDirectory sourceDir;
        AZ::IO::Path sourceFilePath = sourceDir.Resolve(testAssetName);

        // Basic test: test data before and after are same. Test data class doesn't have converter or asset reference.
        AssetBuilderSDK::ProcessJobRequest request;

        // Initial job request
        request.m_fullPath = sourceFilePath.Native();
        request.m_tempDirPath = productDir.GetDirectory();

        // Dummy pass template
        RPI::PassTemplate passTemplate;
        passTemplate.m_name = "TemplateTestName";

        // Dummy pass data with random asset ID to test asset dependency
        AZStd::shared_ptr<RPI::FullscreenTrianglePassData> passData = AZStd::make_shared<RPI::FullscreenTrianglePassData>();
        passData->m_shaderAsset.m_assetId = Data::AssetId(Uuid::CreateRandom(), 1);
        passTemplate.m_passData = passData;

        // Create and write pass data
        RPI::PassAsset passAsset;
        SetPassTemplateForTestingOnly(passAsset, passTemplate);
        JsonSerializationUtils::SaveObjectToFile(&passAsset, sourceFilePath.Native());

        // Process job
        RPI::PassBuilder builder;
        AssetBuilderSDK::ProcessJobResponse response;
        builder.ProcessJob(request, response);

        // Verify job success
        EXPECT_TRUE(response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success);
        EXPECT_TRUE(response.m_outputProducts.size() == 1);

        // Verify input and output names are the same
        Data::Asset<Data::AssetData> readAsset = LoadAssetFromFile(response.m_outputProducts[0].m_productFileName.c_str());
        RPI::PassAsset* readPassAsset = static_cast<RPI::PassAsset*>(readAsset.GetData());
        EXPECT_TRUE(passTemplate.m_name == readPassAsset->GetPassTemplate()->m_name);
    }

} // namespace UnitTests
