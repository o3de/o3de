/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RCBuilderTest.h"


TEST_F(RCBuilderTest, MatchTempFileToSkip_SkipRCFiles_true)
{
    const char* rc_skip_fileNames[] = {
        "rc_createdfiles.txt",
        "rc_log.log",
        "rc_log_warnings.log",
        "rc_log_errors.log"
    };

    for (const char* filename : rc_skip_fileNames)
    {
        ASSERT_TRUE(AssetProcessor::InternalRecognizerBasedBuilder::MatchTempFileToSkip(filename));
    }
}

TEST_F(RCBuilderTest, MatchTempFileToSkip_SkipRCFiles_false)
{
    const char* rc_not_skip_fileNames[] = {
        "foo.log",
        "bar.txt"
    };

    for (const char* filename : rc_not_skip_fileNames)
    {
        ASSERT_FALSE(AssetProcessor::InternalRecognizerBasedBuilder::MatchTempFileToSkip(filename));
    }
}

class MockBuilderListener : public AssetBuilderSDK::AssetBuilderBus::Handler
{
public:
    void RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& builderDesc) override
    {
        m_wasCalled = true;
        m_result = builderDesc;
    }

    bool m_wasCalled = false;
    AssetBuilderSDK::AssetBuilderDesc m_result;
};


class RCBuilderFingerprintTest
    : public RCBuilderTest
{
    
public:

    // A utility function which feeds in the version and asset type to the builder, fingerprints it, and returns the fingerprint
    AZStd::string BuildFingerprint(int versionNumber, AZ::Uuid builderProductType)
    {
        TestInternalRecognizerBasedBuilder test;
        MockRecognizerConfiguration configuration;

        AssetPlatformSpec   good_spec;
        good_spec.m_extraRCParams = "/i";

        AssetRecognizer good;
        good.m_name = "Good";
        good.m_version = static_cast<char>(versionNumber);
        good.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        good.m_platformSpecs["pc"] = good_spec;
        good.m_productAssetType = builderProductType;
        
        configuration.m_recognizerContainer["good"] = good;

        MockBuilderListener listener;
        listener.BusConnect();

        bool initialization_result = test.Initialize(configuration);
        listener.BusDisconnect();

        EXPECT_TRUE(listener.m_wasCalled);
        EXPECT_TRUE(initialization_result);

        EXPECT_STRNE(listener.m_result.m_analysisFingerprint.c_str(), "");
        return listener.m_result.m_analysisFingerprint;
    }
};

TEST_F(RCBuilderFingerprintTest, DifferentVersion_Has_DifferentAnalysisFingerprint)
{
    AZ::Uuid uuid1 = AZ::Uuid::CreateRandom();
    AZStd::string analysisFingerprint1 = BuildFingerprint(1, uuid1);
    AZStd::string analysisFingerprint2 = BuildFingerprint(2, uuid1);
    EXPECT_STRNE(analysisFingerprint1.c_str(), analysisFingerprint2.c_str());
}

TEST_F(RCBuilderFingerprintTest, DifferentAssetType_Has_DifferentAnalysisFingerprint)
{
    AZ::Uuid uuid1 = AZ::Uuid::CreateRandom();
    AZ::Uuid uuid2 = AZ::Uuid::CreateRandom();
    AZStd::string analysisFingerprint1 = BuildFingerprint(1, uuid1);
    AZStd::string analysisFingerprint2 = BuildFingerprint(1, uuid2);
    EXPECT_STRNE(analysisFingerprint1.c_str(), analysisFingerprint2.c_str());
}

TEST_F(RCBuilderFingerprintTest, DifferentAssetTypeAndVersion_Has_DifferentAnalysisFingerprint)
{
    AZ::Uuid uuid1 = AZ::Uuid::CreateRandom();
    AZ::Uuid uuid2 = AZ::Uuid::CreateRandom();
    AZStd::string analysisFingerprint1 = BuildFingerprint(1, uuid1);
    AZStd::string analysisFingerprint2 = BuildFingerprint(2, uuid2);
    EXPECT_STRNE(analysisFingerprint1.c_str(), analysisFingerprint2.c_str());
}

TEST_F(RCBuilderFingerprintTest, SameVersionAndSameType_Has_SameAnalysisFingerprint)
{
    AZ::Uuid uuid1 = AZ::Uuid::CreateRandom();
    AZStd::string analysisFingerprint1 = BuildFingerprint(1, uuid1);
    AZStd::string analysisFingerprint2 = BuildFingerprint(1, uuid1);
    EXPECT_STREQ(analysisFingerprint1.c_str(), analysisFingerprint2.c_str());
}

