/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSMetricsServiceApi.h>
#include <AWSMetricsConstant.h>
#include <MetricsEventBuilder.h>
#include <Framework/JsonObjectHandler.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace AWSMetrics
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

    class AWSMetricsServiceApiTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        testing::NiceMock<JsonReaderMock> JsonReader;
    };

    TEST_F(AWSMetricsServiceApiTest, OnJsonKey_MetricsEventSuccessResponseRecord_AcceptValidKeys)
    {
        ServiceAPI::PostMetricsEventsResponseEntry responseRecord;
        responseRecord.m_result = "ok";

        EXPECT_CALL(JsonReader, Accept(responseRecord.m_result)).Times(1);
        EXPECT_CALL(JsonReader, Accept(responseRecord.m_errorCode)).Times(1);
        EXPECT_CALL(JsonReader, Ignore()).Times(1);

        responseRecord.OnJsonKey(AwsMetricsPostMetricsEventsResponseEntryKeyResult, JsonReader);
        responseRecord.OnJsonKey(AwsMetricsPostMetricsEventsResponseEntryKeyErrorCode, JsonReader);
        responseRecord.OnJsonKey("other", JsonReader);
    }

    TEST_F(AWSMetricsServiceApiTest, OnJsonKeyWithEvents_MetricsEventSuccessResponseRecord_AcceptValidKeys)
    {
        // Verifiy that JsonReader accepts valid JSON keys in each event record from a success reponse 
        ServiceAPI::PostMetricsEventsResponseEntry responseRecord;
        responseRecord.m_result = "Ok";

        ServiceAPI::PostMetricsEventsResponse response;
        response.m_responseEntries.emplace_back(responseRecord);
        response.m_failedRecordCount = 0;
        response.m_total = 1;

        EXPECT_CALL(JsonReader, Accept(response.m_failedRecordCount)).Times(1);
        EXPECT_CALL(JsonReader, Accept(response.m_total)).Times(1);
        EXPECT_CALL(JsonReader, Accept(::testing::An<AWSCore::JsonArrayHandler>())).Times(1);
        EXPECT_CALL(JsonReader, Ignore()).Times(1);

        response.OnJsonKey(AwsMetricsPostMetricsEventsResponseKeyFailedRecordCount, JsonReader);
        response.OnJsonKey(AwsMetricsPostMetricsEventsResponseKeyTotal, JsonReader);
        response.OnJsonKey(AwsMetricsPostMetricsEventsResponseKeyEvents, JsonReader);
        response.OnJsonKey("other", JsonReader);
    }

    TEST_F(AWSMetricsServiceApiTest, OnJsonKey_Error_AcceptValidKeys)
    {
        ServiceAPI::PostMetricsEventsError error;
        error.message = "error message";
        error.type = "404";

        EXPECT_CALL(JsonReader, Accept(error.message)).Times(1);
        EXPECT_CALL(JsonReader, Accept(error.type)).Times(1);
        EXPECT_CALL(JsonReader, Ignore()).Times(1);

        error.OnJsonKey(AwsMetricsPostMetricsEventsErrorKeyMessage, JsonReader);
        error.OnJsonKey(AwsMetricsPostMetricsEventsErrorKeyType, JsonReader);
        error.OnJsonKey("other", JsonReader);
    }

    TEST_F(AWSMetricsServiceApiTest, BuildRequestBody_PostProducerEventsRequest_SerializedMetricsQueue)
    {
        ServiceAPI::PostMetricsEventsRequest request;
        request.parameters.m_metricsQueue = MetricsQueue();
        request.parameters.m_metricsQueue.AddMetrics(MetricsEventBuilder().Build());

        AWSCore::RequestBuilder requestBuilder{};
        EXPECT_TRUE(request.parameters.BuildRequest(requestBuilder));
        std::shared_ptr<Aws::StringStream> bodyContent = requestBuilder.GetBodyContent();
        ASSERT_NE(nullptr, bodyContent);

        std::istreambuf_iterator<AZStd::string::value_type> eos;
        AZStd::string bodyString{ std::istreambuf_iterator<AZStd::string::value_type>(*bodyContent), eos };
        EXPECT_TRUE(bodyString.contains(AZStd::string::format("{\"%s\":[{\"event_timestamp\":", AwsMetricsPostMetricsEventsRequestParameterKeyEvents)));
    }
}
