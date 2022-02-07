/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AWSCoreBus.h>
#include <Framework/ServiceClientJobConfig.h>
#include <ResourceMapping/AWSResourceMappingBus.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

static constexpr const char TEST_EXPECTED_FEATURE_SERVICE_URL[] = "https://feature.service.com";
static constexpr const char TEST_EXPECTED_CUSTOM_SERVICE_URL[] = "https://custom.service.com";

class ServiceClientJobConfigTest
    : public UnitTest::ScopedAllocatorSetupFixture
    , AWSResourceMappingRequestBus::Handler
{
    void SetUp() override
    {
        AWSResourceMappingRequestBus::Handler::BusConnect();
    }

    void TearDown() override
    {
        AWSResourceMappingRequestBus::Handler::BusDisconnect();
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

    AZStd::string GetResourceAccountId(const AZStd::string& resourceKeyName) const override
    {
        AZ_UNUSED(resourceKeyName);
        return "";
    }

    AZStd::string GetResourceNameId(const AZStd::string& resourceKeyName) const override
    {
        AZ_UNUSED(resourceKeyName);
        return "";
    }

    AZStd::string GetResourceRegion(const AZStd::string& resourceKeyName) const override
    {
        AZ_UNUSED(resourceKeyName);
        return "";
    }

    AZStd::string GetResourceType(const AZStd::string& resourceKeyName) const override
    {
        AZ_UNUSED(resourceKeyName);
        return "";
    }

    AZStd::string GetServiceUrlByServiceName(const AZStd::string& serviceName) const override
    {
        AZ_UNUSED(serviceName);
        return TEST_EXPECTED_FEATURE_SERVICE_URL;
    }

    AZStd::string GetServiceUrlByRESTApiIdAndStage(
        const AZStd::string& restApiIdKeyName, const AZStd::string& restApiStageKeyName) const override
    {
        AZ_UNUSED(restApiIdKeyName);
        AZ_UNUSED(restApiStageKeyName);
        return TEST_EXPECTED_CUSTOM_SERVICE_URL;
    }

    void ReloadConfigFile(bool reloadConfigFileName = false) override
    {
        AZ_UNUSED(reloadConfigFileName);
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
