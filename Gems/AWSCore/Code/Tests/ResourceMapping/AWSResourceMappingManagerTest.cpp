/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <Configuration/AWSCoreConfiguration.h>
#include <ResourceMapping/AWSResourceMappingConstants.h>
#include <ResourceMapping/AWSResourceMappingManager.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

static constexpr const char TEST_EXPECTED_GLOBAL_ACCOUNTID[] = "123456789012";
static constexpr const char TEST_EXPECTED_GLOBAL_REGION[] = "us-west-2";
static constexpr const char TEST_EXPECTED_RESOURCE_ACCOUNTID[] = "012345678912";
static constexpr const char TEST_EXPECTED_RESOURCE_REGION[] = "us-east-1";

static constexpr const char TEST_EXPECTED_LAMBDA_KEYNAME[] = "TestLambda";
static constexpr const char TEST_EXPECTED_LAMBDA_TYPE[] = "AWS::Lambda::Function";
static constexpr const char TEST_EXPECTED_LAMBDA_NAMEID[] = "MyTestLambda";
static constexpr const char TEST_EXPECTED_BUCKET_KEYNAME[] = "TestS3Bucket";
static constexpr const char TEST_EXPECTED_BUCKET_TYPE[] = "AWS::S3::Bucket";
static constexpr const char TEST_EXPECTED_BUCKET_NAMEID[] = "MyTestS3Bucket";

static constexpr const char TEST_EXPECTED_SERVICE_KEYNAME[] = "TestService";

static constexpr const char TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE[] =
R"({
    "AWSResourceMappings": {
        "TestLambda": {
            "Type": "AWS::Lambda::Function",
            "Name/ID": "MyTestLambda",
            "Region": "us-east-1",
            "AccountId": "012345678912"
        },
        "TestS3Bucket": {
            "Type": "AWS::S3::Bucket",
            "Name/ID": "MyTestS3Bucket"
        },
        "TestService.RESTApiId": {
            "Type": "AWS::ApiGateway::RestApi",
            "Name/ID": "1234567890"
        },
        "TestService.RESTApiStage": {
            "Type": "AWS::ApiGateway::Stage",
            "Name/ID": "prod",
            "Region": "us-east-1"
        }
    },
    "AccountId": "123456789012",
    "Region": "us-west-2",
    "Version": "1.0.0"
})";

static constexpr const char TEST_VALID_EMPTY_ACCOUNTID_RESOURCE_MAPPING_CONFIG_FILE[] =
    R"({
    "AWSResourceMappings": {
        "TestLambda": {
            "Type": "AWS::Lambda::Function",
            "Name/ID": "MyTestLambda",
            "Region": "us-east-1",
            "AccountId": "012345678912"
        },
        "TestS3Bucket": {
            "Type": "AWS::S3::Bucket",
            "Name/ID": "MyTestS3Bucket"
        },
        "TestService.RESTApiId": {
            "Type": "AWS::ApiGateway::RestApi",
            "Name/ID": "1234567890"
        },
        "TestService.RESTApiStage": {
            "Type": "AWS::ApiGateway::Stage",
            "Name/ID": "prod",
            "Region": "us-east-1"
        }
    },
    "AccountId": "",
    "Region": "us-west-2",
    "Version": "1.1.0"
})";

static constexpr const char TEST_INVALID_RESOURCE_MAPPING_CONFIG_FILE[] =
R"({
    "AWSResourceMappings": {},
    "AccountId": "123",
    "Region": "123",
    "Version": "123"
})";
static constexpr const char TEST_TEMPLATE_RESOURCE_MAPPING_CONFIG_FILE[] =
    R"({
    "AWSResourceMappings": {},
    "AccountId": "EMPTY",
    "Region": "us-west-2",
    "Version": "1.0.0"
})";

class AWSResourceMappingManagerTest
    : public AWSCoreFixture
    , AWSCoreInternalRequestBus::Handler
{
public:
    AWSResourceMappingManagerTest(): m_reloadConfigurationCounter(0)
    {
        m_resourceMappingManager = AZStd::make_unique<AWSCore::AWSResourceMappingManager>();
    }

    void SetUp() override
    {
        AWSCoreFixture::SetUpFixture(false);

        m_configFilePath = (GetTestTempDirectoryPath() /
                            AWSCore::AWSCoreConfiguration::AWSCoreResourceMappingConfigFolderName /
                            "test_aws_resource_mappings.json").LexicallyNormal();
        AWSCoreInternalRequestBus::Handler::BusConnect();
    }

    void TearDown() override
    {
        AWSCoreInternalRequestBus::Handler::BusDisconnect();
        RemoveFile(m_configFilePath.Native());
        m_configFilePath.clear();

        m_reloadConfigurationCounter = 0;
        m_resourceMappingManager->DeactivateManager();
        m_resourceMappingManager.reset();

        AWSCoreFixture::TearDownFixture(false);
    }

    // AWSCoreInternalRequestBus interface implementation
    AZStd::string GetProfileName() const override { return ""; }
    AZStd::string GetResourceMappingConfigFilePath() const override { return m_configFilePath.Native(); }
    bool IsAllowedAWSMetadataCredentials() const override { return false; }
    void ReloadConfiguration() override { m_reloadConfigurationCounter++; }

    AZStd::unique_ptr<AWSCore::AWSResourceMappingManager> m_resourceMappingManager;
    AZ::u8 m_reloadConfigurationCounter;
    AZ::IO::Path m_configFilePath;
};

TEST_F(AWSResourceMappingManagerTest, ActivateManager_ParseInvalidConfigFile_ConfigDataIsEmpty)
{
    CreateFile(m_configFilePath.Native(), TEST_INVALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualAccountId;
    AZStd::string actualRegion;
    AWSResourceMappingRequestBus::BroadcastResult(actualAccountId, &AWSResourceMappingRequests::GetDefaultAccountId);
    AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSResourceMappingRequests::GetDefaultRegion);
    EXPECT_EQ(m_reloadConfigurationCounter, 0);
    EXPECT_TRUE(actualAccountId.empty());
    EXPECT_TRUE(actualRegion.empty());
    EXPECT_TRUE(m_resourceMappingManager->GetStatus() == AWSResourceMappingManager::Status::Error);
}

TEST_F(AWSResourceMappingManagerTest, ActivateManager_ParseValidConfigFile_ConfigDataIsNotEmpty)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualAccountId;
    AZStd::string actualRegion;
    AWSResourceMappingRequestBus::BroadcastResult(actualAccountId, &AWSResourceMappingRequests::GetDefaultAccountId);
    AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSResourceMappingRequests::GetDefaultRegion);
    EXPECT_EQ(m_reloadConfigurationCounter, 0);
    EXPECT_FALSE(actualAccountId.empty());
    EXPECT_FALSE(actualRegion.empty());
    EXPECT_TRUE(m_resourceMappingManager->GetStatus() == AWSResourceMappingManager::Status::Ready);
}

TEST_F(AWSResourceMappingManagerTest, ActivateManager_ParseTemplateConfigFile_ConfigDataIsNotEmpty)
{
    CreateFile(m_configFilePath.Native(), TEST_TEMPLATE_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualAccountId;
    AZStd::string actualRegion;
    AWSResourceMappingRequestBus::BroadcastResult(actualAccountId, &AWSResourceMappingRequests::GetDefaultAccountId);
    AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSResourceMappingRequests::GetDefaultRegion);
    EXPECT_EQ(m_reloadConfigurationCounter, 0);
    EXPECT_FALSE(actualAccountId.empty());
    EXPECT_FALSE(actualRegion.empty());
    EXPECT_TRUE(m_resourceMappingManager->GetStatus() == AWSResourceMappingManager::Status::Ready);
}

TEST_F(AWSResourceMappingManagerTest, ActivateManager_ParseValidConfigFile_ConfigDataIsNotEmptyWithMultithreadCalls)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    constexpr int testThreadNumber = 10;
    AZStd::atomic<int> actualEbusCalls = 0;
    AZStd::vector<AZStd::thread> testThreadPool;
    for (int index = 0; index < testThreadNumber; index++)
    {
        testThreadPool.emplace_back(AZStd::thread([&]() {
            AZStd::string actualAccountId;
            AWSResourceMappingRequestBus::BroadcastResult(actualAccountId, &AWSResourceMappingRequests::GetDefaultAccountId);
            EXPECT_FALSE(actualAccountId.empty());
            ++actualEbusCalls;
        }));
    }

    for (auto& testThread : testThreadPool)
    {
        testThread.join();
    }
    EXPECT_TRUE(actualEbusCalls == testThreadNumber);
}

TEST_F(AWSResourceMappingManagerTest, ActivateManager_ParseValidConfigFile_GlobalAccountIdEmpty)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_EMPTY_ACCOUNTID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualAccountId;
    AZStd::string actualRegion;
    AWSResourceMappingRequestBus::BroadcastResult(actualAccountId, &AWSResourceMappingRequests::GetDefaultAccountId);
    AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSResourceMappingRequests::GetDefaultRegion);
    EXPECT_EQ(m_reloadConfigurationCounter, 0);
    EXPECT_TRUE(actualAccountId.empty());
    EXPECT_FALSE(actualRegion.empty());
    EXPECT_TRUE(m_resourceMappingManager->GetStatus() == AWSResourceMappingManager::Status::Ready);
}

TEST_F(AWSResourceMappingManagerTest, DeactivateManager_AfterActivatingWithValidConfigFile_ConfigDataGetCleanedUp)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualAccountId;
    AZStd::string actualRegion;
    AWSResourceMappingRequestBus::BroadcastResult(actualAccountId, &AWSResourceMappingRequests::GetDefaultAccountId);
    AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSResourceMappingRequests::GetDefaultRegion);
    EXPECT_FALSE(actualAccountId.empty());
    EXPECT_FALSE(actualRegion.empty());
    EXPECT_TRUE(m_resourceMappingManager->GetStatus() == AWSResourceMappingManager::Status::Ready);

    m_resourceMappingManager->DeactivateManager();

    EXPECT_TRUE(m_resourceMappingManager->GetDefaultAccountId().empty());
    EXPECT_TRUE(m_resourceMappingManager->GetDefaultRegion().empty());
    EXPECT_TRUE(m_resourceMappingManager->GetStatus() == AWSResourceMappingManager::Status::NotLoaded);
}

TEST_F(AWSResourceMappingManagerTest, GetDefaultAccountId_AfterParsingValidConfigFile_GetExpectedDefaultAccountId)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualAccountId;
    AWSResourceMappingRequestBus::BroadcastResult(actualAccountId, &AWSResourceMappingRequests::GetDefaultAccountId);
    EXPECT_FALSE(actualAccountId.empty());
    EXPECT_EQ(TEST_EXPECTED_GLOBAL_ACCOUNTID, actualAccountId);
}

TEST_F(AWSResourceMappingManagerTest, GetDefaultRegion_AfterParsingValidConfigFile_GetExpectedDefaultRegion)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualRegion;
    AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSResourceMappingRequests::GetDefaultRegion);
    EXPECT_FALSE(actualRegion.empty());
    EXPECT_EQ(TEST_EXPECTED_GLOBAL_REGION, actualRegion);
}

TEST_F(AWSResourceMappingManagerTest, GetResourceAccountId_AfterParsingValidConfigFile_GetExpectedAccountId)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualAccountId;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualAccountId, &AWSResourceMappingRequests::GetResourceAccountId, TEST_EXPECTED_LAMBDA_KEYNAME);
    EXPECT_FALSE(actualAccountId.empty());
    EXPECT_EQ(TEST_EXPECTED_RESOURCE_ACCOUNTID, actualAccountId);

    AWSResourceMappingRequestBus::BroadcastResult(
        actualAccountId, &AWSResourceMappingRequests::GetResourceAccountId, TEST_EXPECTED_BUCKET_KEYNAME);
    EXPECT_FALSE(actualAccountId.empty());
    EXPECT_EQ(TEST_EXPECTED_GLOBAL_ACCOUNTID, actualAccountId);
}

TEST_F(AWSResourceMappingManagerTest, GetResourceAccountId_QueryNonexistResourceMappingKeyName_GetEmptyAccountId)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualAccountId;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualAccountId, &AWSResourceMappingRequests::GetResourceAccountId, "dummyKeyName");
    EXPECT_TRUE(actualAccountId.empty());
}

TEST_F(AWSResourceMappingManagerTest, GetResourceNameId_AfterParsingValidConfigFile_GetExpectedNameId)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualNameId;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualNameId, &AWSResourceMappingRequests::GetResourceNameId, TEST_EXPECTED_LAMBDA_KEYNAME);
    EXPECT_FALSE(actualNameId.empty());
    EXPECT_EQ(TEST_EXPECTED_LAMBDA_NAMEID, actualNameId);

    AWSResourceMappingRequestBus::BroadcastResult(
        actualNameId, &AWSResourceMappingRequests::GetResourceNameId, TEST_EXPECTED_BUCKET_KEYNAME);
    EXPECT_FALSE(actualNameId.empty());
    EXPECT_EQ(TEST_EXPECTED_BUCKET_NAMEID, actualNameId);
}

TEST_F(AWSResourceMappingManagerTest, GetResourceNameId_QueryNonexistResourceMappingKeyName_GetEmptyNameId)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualNameId;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualNameId, &AWSResourceMappingRequests::GetResourceNameId, "dummyKeyName");
    EXPECT_TRUE(actualNameId.empty());
}

TEST_F(AWSResourceMappingManagerTest, GetResourceRegion_AfterParsingValidConfigFile_GetExpectedRegion)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualRegion;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualRegion, &AWSResourceMappingRequests::GetResourceRegion, TEST_EXPECTED_LAMBDA_KEYNAME);
    EXPECT_FALSE(actualRegion.empty());
    EXPECT_EQ(TEST_EXPECTED_RESOURCE_REGION, actualRegion);

    AWSResourceMappingRequestBus::BroadcastResult(
        actualRegion, &AWSResourceMappingRequests::GetResourceRegion, TEST_EXPECTED_BUCKET_KEYNAME);
    EXPECT_FALSE(actualRegion.empty());
    EXPECT_EQ(TEST_EXPECTED_GLOBAL_REGION, actualRegion);
}

TEST_F(AWSResourceMappingManagerTest, GetResourceRegion_QueryNonexistResourceMappingKeyName_GetEmptyRegion)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualRegion;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualRegion, &AWSResourceMappingRequests::GetResourceRegion, "dummyKeyName");
    EXPECT_TRUE(actualRegion.empty());
}

TEST_F(AWSResourceMappingManagerTest, GetResourceType_AfterParsingValidConfigFile_GetExpectedType)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualType;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualType, &AWSResourceMappingRequests::GetResourceType, TEST_EXPECTED_LAMBDA_KEYNAME);
    EXPECT_FALSE(actualType.empty());
    EXPECT_EQ(TEST_EXPECTED_LAMBDA_TYPE, actualType);

    AWSResourceMappingRequestBus::BroadcastResult(
        actualType, &AWSResourceMappingRequests::GetResourceType, TEST_EXPECTED_BUCKET_KEYNAME);
    EXPECT_FALSE(actualType.empty());
    EXPECT_EQ(TEST_EXPECTED_BUCKET_TYPE, actualType);
}

TEST_F(AWSResourceMappingManagerTest, GetResourceType_QueryNonexistResourceMappingKeyName_GetEmptyType)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualType;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualType, &AWSResourceMappingRequests::GetResourceType, "dummyKeyName");
    EXPECT_TRUE(actualType.empty());
}

TEST_F(AWSResourceMappingManagerTest, GetServiceUrl_PassingEmptyServiceName_GetEmptyUrl)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualServiceUrl;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualServiceUrl, &AWSResourceMappingRequests::GetServiceUrlByServiceName, "");
    EXPECT_TRUE(actualServiceUrl.empty());
}

TEST_F(AWSResourceMappingManagerTest, GetServiceUrl_PassingEmptyRESTApiIdAndStage_GetEmptyUrl)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualServiceUrl;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualServiceUrl, &AWSResourceMappingRequests::GetServiceUrlByRESTApiIdAndStage, "", "");
    EXPECT_TRUE(actualServiceUrl.empty());
}

TEST_F(AWSResourceMappingManagerTest, GetServiceUrl_RESTApiIdAndStageHaveInconsistentRegion_GetEmptyUrl)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualServiceUrl;
    AWSResourceMappingRequestBus::BroadcastResult(
        actualServiceUrl, &AWSResourceMappingRequests::GetServiceUrlByServiceName, TEST_EXPECTED_SERVICE_KEYNAME);
    EXPECT_TRUE(actualServiceUrl.empty());
}

TEST_F(AWSResourceMappingManagerTest, ReloadConfigFile_ParseValidConfigFileAfterParsingInvalid_ConfigDataGetParsed)
{
    CreateFile(m_configFilePath.Native(), TEST_INVALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ActivateManager();

    AZStd::string actualAccountId;
    AZStd::string actualRegion;
    AWSResourceMappingRequestBus::BroadcastResult(actualAccountId, &AWSResourceMappingRequests::GetDefaultAccountId);
    AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSResourceMappingRequests::GetDefaultRegion);
    EXPECT_EQ(m_reloadConfigurationCounter, 0);
    EXPECT_TRUE(actualAccountId.empty());
    EXPECT_TRUE(actualRegion.empty());
    EXPECT_TRUE(m_resourceMappingManager->GetStatus() == AWSResourceMappingManager::Status::Error);

    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ReloadConfigFile();

    AWSResourceMappingRequestBus::BroadcastResult(actualAccountId, &AWSResourceMappingRequests::GetDefaultAccountId);
    AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSResourceMappingRequests::GetDefaultRegion);
    EXPECT_EQ(m_reloadConfigurationCounter, 0);
    EXPECT_FALSE(actualAccountId.empty());
    EXPECT_FALSE(actualRegion.empty());
    EXPECT_TRUE(m_resourceMappingManager->GetStatus() == AWSResourceMappingManager::Status::Ready);
}

TEST_F(AWSResourceMappingManagerTest, ReloadConfigFile_ReloadConfigFileNameAndParseValidConfigFile_ConfigDataGetParsed)
{
    CreateFile(m_configFilePath.Native(), TEST_VALID_RESOURCE_MAPPING_CONFIG_FILE);
    m_resourceMappingManager->ReloadConfigFile(true);

    EXPECT_EQ(m_reloadConfigurationCounter, 1);
    EXPECT_FALSE(m_resourceMappingManager->GetDefaultAccountId().empty());
    EXPECT_FALSE(m_resourceMappingManager->GetDefaultRegion().empty());
    EXPECT_TRUE(m_resourceMappingManager->GetStatus() == AWSResourceMappingManager::Status::Ready);
}

TEST_F(AWSResourceMappingManagerTest, ReloadConfigFile_MissingSetRegFile_ConfigDataIsNotParsed)
{
    m_configFilePath.clear();
    m_resourceMappingManager->ReloadConfigFile(true);

    EXPECT_EQ(m_reloadConfigurationCounter, 1);
    EXPECT_TRUE(m_resourceMappingManager->GetDefaultAccountId().empty());
    EXPECT_TRUE(m_resourceMappingManager->GetDefaultRegion().empty());
    EXPECT_TRUE(m_resourceMappingManager->GetStatus() == AWSResourceMappingManager::Status::NotLoaded);
}
