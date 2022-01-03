/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Attribution/AWSCoreAttributionConstant.h>
#include <Editor/Attribution/AWSCoreAttributionMetric.h>
#include <Editor/Attribution/AWSAttributionServiceApi.h>
#include <Framework/JsonObjectHandler.h>

#include <AzCore/UnitTest/TestTypes.h>

using namespace AWSCore;

namespace AWSCoreUnitTest
{
    class JsonReaderMock
        : public AWSCore::JsonReader
    {
    public:
        virtual ~JsonReaderMock() = default;

        MOCK_METHOD0(Ignore, bool());
        MOCK_METHOD1(Accept, bool(bool& target));
        MOCK_METHOD1(Accept, bool(AZStd::string& target));
        MOCK_METHOD1(Accept, bool(int& target));
        MOCK_METHOD1(Accept, bool(unsigned& target));
        MOCK_METHOD1(Accept, bool(int64_t& target));
        MOCK_METHOD1(Accept, bool(uint64_t& target));
        MOCK_METHOD1(Accept, bool(double& target));
        MOCK_METHOD1(Accept, bool(AWSCore::JsonKeyHandler keyHandler));
        MOCK_METHOD1(Accept, bool(AWSCore::JsonArrayHandler arrayHandler));
    };

    class AWSAttributionServiceApiTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        testing::NiceMock<JsonReaderMock> JsonReader;
    };

    TEST_F(AWSAttributionServiceApiTest, AWSAtrributionSuccessResponse_Serialization)
    {
        ServiceAPI::AWSAtrributionSuccessResponse response;
        response.result = "ok";

        EXPECT_CALL(JsonReader, Accept(response.result)).Times(1);
        EXPECT_CALL(JsonReader, Ignore()).Times(0);

        response.OnJsonKey("statusCode", JsonReader);
    }

    TEST_F(AWSAttributionServiceApiTest, AWSAtrributionSuccessResponse_Serialization_Ignore)
    {
        ServiceAPI::AWSAtrributionSuccessResponse response;
        response.result = "ok";

        EXPECT_CALL(JsonReader, Accept(response.result)).Times(0);
        EXPECT_CALL(JsonReader, Ignore()).Times(1);

        response.OnJsonKey("", JsonReader);
    }

    TEST_F(AWSAttributionServiceApiTest, BuildRequestBody_PostProducerEventsRequest_SerializedMetricsQueue)
    {
        ServiceAPI::AWSAttributionRequest request;
        request.parameters.metric = AttributionMetric();

        AWSCore::RequestBuilder requestBuilder{};
        EXPECT_TRUE(request.parameters.BuildRequest(requestBuilder));
        std::shared_ptr<Aws::StringStream> bodyContent = requestBuilder.GetBodyContent();
        EXPECT_TRUE(bodyContent != nullptr);

        AZStd::string bodyString;
        std::istreambuf_iterator<AZStd::string::value_type> eos;
        bodyString = AZStd::string{ std::istreambuf_iterator<AZStd::string::value_type>(*bodyContent), eos };
        AZ_Printf("AWSAttributionServiceApiTest", bodyString.c_str());
        EXPECT_TRUE(bodyString.find(AZStd::string::format("{\"%s\":\"1.1\"", AwsAttributionAttributeKeyVersion)) != AZStd::string::npos);
    }
}
