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
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        testing::NiceMock<JsonReaderMock> JsonReader;
    };

    TEST_F(AWSMetricsServiceApiTest, OnJsonKey_MetricsEventSuccessResponseRecord_AcceptValidKeys)
    {
        ServiceAPI::MetricsEventSuccessResponseRecord responseRecord;
        responseRecord.result = "ok";

        EXPECT_CALL(JsonReader, Accept(responseRecord.result)).Times(1);
        EXPECT_CALL(JsonReader, Accept(responseRecord.errorCode)).Times(1);
        EXPECT_CALL(JsonReader, Ignore()).Times(1);

        responseRecord.OnJsonKey(SUCCESS_RESPONSE_RECORD_KEY_RESULT, JsonReader);
        responseRecord.OnJsonKey(SUCCESS_RESPONSE_RECORD_KEY_ERROR_CODE, JsonReader);
        responseRecord.OnJsonKey("other", JsonReader);
    }

    TEST_F(AWSMetricsServiceApiTest, OnJsonKeyWithEvents_MetricsEventSuccessResponseRecord_AcceptValidKeys)
    {
        // Verifiy that JsonReader accepts valid JSON keys in each event record from a success reponse 
        ServiceAPI::MetricsEventSuccessResponseRecord responseRecord;
        responseRecord.result = "ok";

        ServiceAPI::MetricsEventSuccessResponse response;
        response.events.emplace_back(responseRecord);
        response.failedRecordCount = 0;
        response.total = 1;

        EXPECT_CALL(JsonReader, Accept(response.failedRecordCount)).Times(1);
        EXPECT_CALL(JsonReader, Accept(response.total)).Times(1);
        EXPECT_CALL(JsonReader, Accept(::testing::An<AWSCore::JsonArrayHandler>())).Times(1);
        EXPECT_CALL(JsonReader, Ignore()).Times(1);

        response.OnJsonKey(SUCCESS_RESPONSE_KEY_FAILED_RECORD_COUNT, JsonReader);
        response.OnJsonKey(SUCCESS_RESPONSE_KEY_TOTAL, JsonReader);
        response.OnJsonKey(SUCCESS_RESPONSE_KEY_EVENTS, JsonReader);
        response.OnJsonKey("other", JsonReader);
    }

    TEST_F(AWSMetricsServiceApiTest, OnJsonKey_Error_AcceptValidKeys)
    {
        ServiceAPI::Error error;
        error.message = "error message";
        error.type = "404";

        EXPECT_CALL(JsonReader, Accept(error.message)).Times(1);
        EXPECT_CALL(JsonReader, Accept(error.type)).Times(1);
        EXPECT_CALL(JsonReader, Ignore()).Times(1);

        error.OnJsonKey(ERROR_KEY_MESSAGE, JsonReader);
        error.OnJsonKey(ERROR_KEY_TYPE, JsonReader);
        error.OnJsonKey("other", JsonReader);
    }

    TEST_F(AWSMetricsServiceApiTest, BuildRequestBody_PostProducerEventsRequest_SerializedMetricsQueue)
    {
        ServiceAPI::PostProducerEventsRequest request;
        request.parameters.data = MetricsQueue();
        request.parameters.data.AddMetrics(MetricsEventBuilder().Build());

        AWSCore::RequestBuilder requestBuilder{};
        EXPECT_TRUE(request.parameters.BuildRequest(requestBuilder));
        std::shared_ptr<Aws::StringStream> bodyContent = requestBuilder.GetBodyContent();
        EXPECT_TRUE(bodyContent != nullptr);

        AZStd::string bodyString;
        std::istreambuf_iterator<AZStd::string::value_type> eos;
        bodyString = AZStd::string{ std::istreambuf_iterator<AZStd::string::value_type>(*bodyContent), eos };
        EXPECT_TRUE(bodyString.find(AZStd::string::format("{\"%s\":[{\"event_timestamp\":", REQUEST_PARAMETER_KEY_EVENTS)) != AZStd::string::npos);
    }
}
