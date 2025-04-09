/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/utilities/BuilderConfigurationManager.h>
#include <native/unittests/UnitTestUtils.h>
#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif

#include <QTemporaryDir>


class BuilderConfigurationTests
    : public ::UnitTest::LeakDetectionFixture
{
public:
    BuilderConfigurationTests()
    {

    }
    virtual ~BuilderConfigurationTests()
    {
    }

    void SetUp() override
    {

    }
    void TearDown() override
    {

    }

    void CreateTestConfig(QString iniStr, AssetProcessor::BuilderConfigurationManager& configurationManager)
    {
        QDir tempPath(m_tempDir.path());
        UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath(AssetProcessor::BuilderConfigFile).toUtf8().data(), iniStr);
        configurationManager.LoadConfiguration(tempPath.absoluteFilePath(AssetProcessor::BuilderConfigFile).toUtf8().data());
    }

    QTemporaryDir m_tempDir;

};

const char SampleConfig[] = 
"[Job PNG Compile]\n"
"checkServer=true\n"
"priority=3\n"
"critical=true\n"
"checkExclusiveLock=true\n"
"fingerprint=finger\n"
"jobFingerprint=somejob7\n"
"params=something=true,otherthing,somethingelse=7\n"

"[Builder Image Worker Builder]\n"
"fingerprint=fingerprint11\n"
"version=7\n"
"patterns=*.png\n"

"[Job TIFF Compile]\n"
"checkServer=false\n"
"priority=9\n"
"critical=false\n"
"checkExclusiveLock=true\n"
"fingerprint=fingerprint1\n"
"params=something=false,otheing,somethingelse=6\n";

TEST_F(BuilderConfigurationTests, TestBuilderConfig_LoadConfig_Success)
{
    AssetProcessor::BuilderConfigurationManager builderConfig;
    CreateTestConfig(SampleConfig, builderConfig);
    ASSERT_TRUE(builderConfig.IsLoaded());
}

TEST_F(BuilderConfigurationTests, TestBuilderConfig_InvalidKey_NoUpdate)
{
    AssetProcessor::BuilderConfigurationManager builderConfig;
    CreateTestConfig(SampleConfig, builderConfig);

    AssetBuilderSDK::JobDescriptor baseDescriptor;
    AssetBuilderSDK::JobDescriptor testDescriptor;

    // Verify an undefined key does not update our data
    ASSERT_FALSE(builderConfig.UpdateJobDescriptor("False Key", testDescriptor));
    ASSERT_EQ(testDescriptor.m_checkServer, baseDescriptor.m_checkServer);
    ASSERT_EQ(testDescriptor.m_critical, baseDescriptor.m_critical);
    ASSERT_EQ(testDescriptor.m_priority, baseDescriptor.m_priority);
    ASSERT_EQ(testDescriptor.m_checkExclusiveLock, baseDescriptor.m_checkExclusiveLock);
    ASSERT_EQ(testDescriptor.m_additionalFingerprintInfo, baseDescriptor.m_additionalFingerprintInfo);
    ASSERT_EQ(testDescriptor.m_jobParameters, baseDescriptor.m_jobParameters);
}

TEST_F(BuilderConfigurationTests, TestBuilderConfig_JobEntry_Success)
{
    AssetProcessor::BuilderConfigurationManager builderConfig;
    CreateTestConfig(SampleConfig, builderConfig);

    AssetBuilderSDK::JobDescriptor testDescriptor;

    // Verify a JobEntry makes the expected updates from data
    ASSERT_TRUE(builderConfig.UpdateJobDescriptor("PNG Compile", testDescriptor));
    ASSERT_EQ(testDescriptor.m_checkServer, true);
    ASSERT_EQ(testDescriptor.m_critical, true);
    ASSERT_EQ(testDescriptor.m_priority, 3);
    ASSERT_EQ(testDescriptor.m_checkExclusiveLock, true);
    ASSERT_EQ(testDescriptor.m_additionalFingerprintInfo, "finger");
    ASSERT_EQ(testDescriptor.m_jobParameters[AZ_CRC_CE("something")], "true");
    ASSERT_EQ(testDescriptor.m_jobParameters[AZ_CRC_CE("somethingelse")], "7");
    ASSERT_NE(testDescriptor.m_jobParameters.find(AZ_CRC_CE("otherthing")), testDescriptor.m_jobParameters.end());
}

TEST_F(BuilderConfigurationTests, TestBuilderConfig_SecondJobEntry_Success)
{
    AssetProcessor::BuilderConfigurationManager builderConfig;
    CreateTestConfig(SampleConfig, builderConfig);

    AssetBuilderSDK::JobDescriptor testDescriptor;

    // Verify a second JobEntry defined in an .ini file makes the expected updates from data
    ASSERT_TRUE(builderConfig.UpdateJobDescriptor("TIFF Compile", testDescriptor));
    ASSERT_EQ(testDescriptor.m_checkServer, false);
    ASSERT_EQ(testDescriptor.m_critical, false);
    ASSERT_EQ(testDescriptor.m_priority, 9);
    ASSERT_EQ(testDescriptor.m_checkExclusiveLock, true);
    ASSERT_EQ(testDescriptor.m_additionalFingerprintInfo, "fingerprint1");
    ASSERT_EQ(testDescriptor.m_jobParameters[AZ_CRC_CE("something")], "false");
    ASSERT_EQ(testDescriptor.m_jobParameters[AZ_CRC_CE("somethingelse")], "6");
    ASSERT_NE(testDescriptor.m_jobParameters.find(AZ_CRC_CE("otheing")), testDescriptor.m_jobParameters.end());
}

TEST_F(BuilderConfigurationTests, TestBuilderConfig_BuilderEntry_Success)
{
    AssetProcessor::BuilderConfigurationManager builderConfig;
    CreateTestConfig(SampleConfig, builderConfig);

    // Verify a Builder makes the expected updates from data
    AssetBuilderSDK::AssetBuilderDesc testBuilder;
    ASSERT_TRUE(builderConfig.UpdateBuilderDescriptor("Image Worker Builder", testBuilder));
    ASSERT_EQ(testBuilder.m_analysisFingerprint, "fingerprint11");
    ASSERT_EQ(testBuilder.m_version, 7);
    ASSERT_EQ(testBuilder.m_patterns.size(), 1);
    ASSERT_EQ(testBuilder.m_patterns[0].m_pattern, "*.png");
}
