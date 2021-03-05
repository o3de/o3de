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

#include "ResourceCompilerLegacy_precompiled.h"
#include "LegacyCompiler.h"
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>

class ResourceCompilerLegacyTest
    : public ::testing::Test
    , public AZ::RC::LegacyCompiler
{
protected:
    void SetUp() override
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        m_app.reset(aznew AzFramework::Application());
        AZ::ComponentApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;
        m_app->Start(desc);

        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        AssetBuilderSDK::InitializeSerializationContext();
    }

    void TearDown() override
    {
        m_app->Destroy();
        m_app.reset();

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

    AZStd::unique_ptr<AssetBuilderSDK::ProcessJobRequest> WrapReadJobRequest(const char* folder) const
    {
        return ReadJobRequest(folder);
    }

    bool WrapWriteResponse(const char* folder, AssetBuilderSDK::ProcessJobResponse& response, bool success = true) const
    {
        return WriteResponse(folder, response, success);
    }

    AZStd::unique_ptr<AzFramework::Application> m_app;
};

TEST_F(ResourceCompilerLegacyTest, ReadJobRequest_NoRequestFile_GenerateProcessJobRequest)
{
    // Tests without ProcessJobRequest.xml
    const char* testFileFolder = "@devroot@/Code/Tools/RC/ResourceCompilerLegacy/Tests";
    char testFileResolvedPath[AZ_MAX_PATH_LEN];
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(testFileFolder, testFileResolvedPath, AZ_ARRAY_SIZE(testFileResolvedPath));

    AZ_TEST_START_TRACE_SUPPRESSION;
    ASSERT_FALSE(WrapReadJobRequest(testFileResolvedPath));
    // Expected error:
    //  Unable to load ProcessJobRequest. Not enough information to process this file
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
}

TEST_F(ResourceCompilerLegacyTest, ReadJobRequest_ValidRequestFile_GenerateProcessJobRequest)
{
    // Tests reading ProcessJobRequest.xml
    const char* testFileFolder = "@devroot@/Code/Tools/RC/ResourceCompilerLegacy/Tests/Output";
    char testFileResolvedFolder[AZ_MAX_PATH_LEN];
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(testFileFolder, testFileResolvedFolder, AZ_ARRAY_SIZE(testFileResolvedFolder));

    ASSERT_TRUE(WrapReadJobRequest(testFileResolvedFolder));
}

TEST_F(ResourceCompilerLegacyTest, WriteJobResponse_ValidProcessJobResponse_WriteProcessJobResponseToDisk)
{
    // Tests writing ProcessJobResponse.xml to disk

    const char* testFileFolder = "@assets@/Code/Tools/RC/ResourceCompilerLegacy/Tests/Output";
    char testFileResolvedFolder[AZ_MAX_PATH_LEN];
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(testFileFolder, testFileResolvedFolder, AZ_ARRAY_SIZE(testFileResolvedFolder));

    AssetBuilderSDK::ProcessJobResponse response;
    ASSERT_TRUE(WrapWriteResponse(testFileResolvedFolder, response));

    AZStd::string responseFilePath;
    AzFramework::StringFunc::Path::ConstructFull(testFileResolvedFolder, AssetBuilderSDK::s_processJobResponseFileName, responseFilePath);
    ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(responseFilePath.c_str()));
    ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Remove(responseFilePath.c_str()));
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);


