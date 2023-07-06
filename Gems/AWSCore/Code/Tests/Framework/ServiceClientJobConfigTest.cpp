/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSCoreBus.h>
#include <Framework/ServiceClientJobConfig.h>
#include <ResourceMapping/AWSResourceMappingBus.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

static constexpr const char TEST_EXPECTED_FEATURE_SERVICE_URL[] = "https://feature.service.com";
static constexpr const char TEST_EXPECTED_CUSTOM_SERVICE_URL[] = "https://custom.service.com";

class ServiceClientJobConfigTest
    : public AWSCoreFixture
    , AWSResourceMappingRequestBus::Handler
{
    void SetUp() override
    {
        AWSCoreFixture::SetUpFixture();

        AWSResourceMappingRequestBus::Handler::BusConnect();
    }

    void TearDown() override
    {
        AWSResourceMappingRequestBus::Handler::BusDisconnect();

        AWSCoreFixture::TearDownFixture();
    }

    // AWSResourceMappingRequestBus interface implementation
    AZStd::string GetDefaultAccountId() const override
    {
        return "";
    }

    AZStd::string GetDefaultRegion() const override
    {
        return "";
    }

    bool HasResource([[maybe_unused]] const AZStd::string& resourceKeyName) const override
    {
        return false;
    }

    AZStd::string GetResourceAccountId([[maybe_unused]] const AZStd::string& resourceKeyName) const override
    {
        return "";
    }

    AZStd::string GetResourceNameId([[maybe_unused]] const AZStd::string& resourceKeyName) const override
    {
        return "";
    }

    AZStd::string GetResourceRegion([[maybe_unused]] const AZStd::string& resourceKeyName) const override
    {
        return "";
    }

    AZStd::string GetResourceType([[maybe_unused]] const AZStd::string& resourceKeyName) const override
    {
        return "";
    }

    AZStd::string GetServiceUrlByServiceName([[maybe_unused]] const AZStd::string& serviceName) const override
    {
        return TEST_EXPECTED_FEATURE_SERVICE_URL;
    }

    AZStd::string GetServiceUrlByRESTApiIdAndStage(
        [[maybe_unused]] const AZStd::string& restApiIdKeyName, [[maybe_unused]] const AZStd::string& restApiStageKeyName) const override
    {
        return TEST_EXPECTED_CUSTOM_SERVICE_URL;
    }

    void ReloadConfigFile([[maybe_unused]] bool reloadConfigFileName = false) override
    {
    }
};

TEST_F(ServiceClientJobConfigTest, GetServiceUrl_CreateServiceWithServiceNameOnly_GetExpectedFeatureServiceUrl)
{
    AWS_SERVICE_TRAITS_TEMPLATE(MyTestService, nullptr, nullptr);
    ServiceClientJobConfig<MyTestServiceServiceTraits> testServiceClientJobConfig;

    auto actualServiceUrl = testServiceClientJobConfig.GetServiceUrl();
    EXPECT_TRUE(actualServiceUrl == TEST_EXPECTED_FEATURE_SERVICE_URL);
}

TEST_F(ServiceClientJobConfigTest, GetServiceUrl_CreateServiceWithApiIdAndStageName_GetExpectedCustomServiceUrl)
{
    AWS_SERVICE_TRAITS_TEMPLATE(MyTestService, "dummyId", "dummyStage");
    ServiceClientJobConfig<MyTestServiceServiceTraits> testServiceClientJobConfig;

    auto actualServiceUrl = testServiceClientJobConfig.GetServiceUrl();
    EXPECT_TRUE(actualServiceUrl == TEST_EXPECTED_CUSTOM_SERVICE_URL);
}
