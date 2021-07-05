/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSMetricsConstant.h>
#include <MetricsEvent.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace AWSMetrics
{
    class MetricsEventTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        static constexpr int NumTestMetrics = 10;
        static const char* const AttrName;
        static const char* const AttrValue;

        AZStd::vector<MetricsAttribute> GetRequiredMetricsAttributes()
        {
            AZStd::vector<MetricsAttribute> result;
            result.emplace_back(MetricsAttribute(AwsMetricsAttributeKeyClientId, "0.0.0.0-{00000000-0000-1000-A000-000000000000}"));
            result.emplace_back(MetricsAttribute(AwsMetricsAttributeKeyEventId, "{00000000-0000-1000-A000-000000000000}"));
            result.emplace_back(MetricsAttribute(AwsMetricsAttributeKeyEventName, "test_event"));
            result.emplace_back(MetricsAttribute(AwsMetricsAttributeKeyEventTimestamp, "0000-00-00T00:00:00Z"));

            return AZStd::move(result);
        }
    };

    const char* const MetricsEventTest::AttrName = "name";
    const char* const MetricsEventTest::AttrValue = "value";

    TEST_F(MetricsEventTest, AddAttribute_SingleAttribute_Success)
    {
        MetricsEvent metrics;
        int numAttributes = metrics.GetNumAttributes();

        metrics.AddAttribute(MetricsAttribute(AttrName, AttrValue));

        ASSERT_EQ(metrics.GetNumAttributes(), numAttributes + 1);
    }

    TEST_F(MetricsEventTest, AddAttribute_DuplicateAttribute_Fail)
    {
        MetricsEvent metrics;
        int numAttributes = metrics.GetNumAttributes();

        metrics.AddAttribute(MetricsAttribute(AttrName, AttrValue));
        AZ_TEST_START_TRACE_SUPPRESSION;
        metrics.AddAttribute(MetricsAttribute(AttrName, AttrValue));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        ASSERT_EQ(metrics.GetNumAttributes(), numAttributes + 1);
    }

    TEST_F(MetricsEventTest, AddAttribute_NoAttributeName_Fail)
    {
        MetricsEvent metrics;
        int numAttributes = metrics.GetNumAttributes();

        AZ_TEST_START_TRACE_SUPPRESSION;
        metrics.AddAttribute(MetricsAttribute());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        ASSERT_EQ(metrics.GetNumAttributes(), numAttributes);
    }

    TEST_F(MetricsEventTest, SetAttributes_ListOfAttributes_Success)
    {
        MetricsEvent metrics;
        AZStd::vector<MetricsAttribute> attributes;
        for (int index = 0; index < NumTestMetrics; ++index)
        {
            attributes.emplace_back(MetricsAttribute(AZStd::string::format("%s%i", AttrName, index), AttrValue));
        }

        metrics.AddAttributes(attributes);

        ASSERT_EQ(metrics.GetNumAttributes(), NumTestMetrics);
    }

    TEST_F(MetricsEventTest, GetSizeInBytes_SingleAttribute_Success)
    {
        MetricsEvent metrics;
        MetricsAttribute attribute(AttrName, AttrValue);
        metrics.AddAttribute(attribute);

        ASSERT_EQ(metrics.GetSizeInBytes(), attribute.GetSizeInBytes());
    }

    TEST_F(MetricsEventTest, SerializeToJson_DefaultAndCustomAttributes_Success)
    {
        MetricsEvent metrics;
        metrics.AddAttribute(MetricsAttribute(AttrName, AttrValue));
        metrics.AddAttribute(MetricsAttribute(AwsMetricsAttributeKeyEventName, AttrValue));

        std::ostream stream(nullptr);
        AWSCore::JsonOutputStream jsonStream{ stream };
        AWSCore::JsonWriter writer{ jsonStream };

        ASSERT_TRUE(metrics.SerializeToJson(writer));
    }

    TEST_F(MetricsEventTest, ReadFromJson_DefaultAndCustomAttributes_Success)
    {
        MetricsEvent metrics;

        rapidjson::Document doc;
        rapidjson::Value metricsObjVal(rapidjson::kObjectType);
        metricsObjVal.AddMember(rapidjson::StringRef(AwsMetricsAttributeKeyEventName), rapidjson::StringRef(AttrValue), doc.GetAllocator());

        rapidjson::Value customEventDataObjVal(rapidjson::kObjectType);
        customEventDataObjVal.AddMember(rapidjson::StringRef(AttrName), rapidjson::StringRef(AttrValue), doc.GetAllocator());
        metricsObjVal.AddMember(rapidjson::Value(AwsMetricsAttributeKeyEventData, doc.GetAllocator()).Move(),
            customEventDataObjVal.Move(), doc.GetAllocator());

        ASSERT_TRUE(metrics.ReadFromJson(metricsObjVal));
        ASSERT_EQ(metrics.GetNumAttributes(), 2);
    }

    TEST_F(MetricsEventTest, ReadFromJson_InvalidJsonValue_Fail)
    {
        MetricsEvent metrics;

        rapidjson::Document doc;
        rapidjson::Value metricsObjVal(rapidjson::kNumberType);

        AZ_TEST_START_TRACE_SUPPRESSION;
        ASSERT_FALSE(metrics.ReadFromJson(metricsObjVal));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(MetricsEventTest, ReadFromJson_InvalidEventData_Fail)
    {
        MetricsEvent metrics;

        rapidjson::Document doc;
        rapidjson::Value metricsObjVal(rapidjson::kObjectType);
        metricsObjVal.AddMember(rapidjson::StringRef(AwsMetricsAttributeKeyEventName), rapidjson::StringRef(AttrValue), doc.GetAllocator());

        rapidjson::Value customEventDataVal(rapidjson::kNumberType);
        metricsObjVal.AddMember(rapidjson::Value(AwsMetricsAttributeKeyEventData, doc.GetAllocator()).Move(),
            customEventDataVal.Move(), doc.GetAllocator());

        AZ_TEST_START_TRACE_SUPPRESSION;
        ASSERT_FALSE(metrics.ReadFromJson(metricsObjVal));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(MetricsEventTest, ValidateAgainstSchema_InvalidMetricsAttributeFormat_Fail)
    {
        AZStd::vector<MetricsAttribute> metricsAttributes = GetRequiredMetricsAttributes();
        metricsAttributes[1].SetVal("InvalidClientId");

        MetricsEvent metrics;
        metrics.AddAttributes(metricsAttributes);

        ASSERT_FALSE(metrics.ValidateAgainstSchema());
    }

    TEST_F(MetricsEventTest, ValidateAgainstSchema_MissingRequiredMetricsAttribute_Fail)
    {
        AZStd::vector<MetricsAttribute> metricsAttributes = GetRequiredMetricsAttributes();
        metricsAttributes.pop_back();

        MetricsEvent metrics;
        metrics.AddAttributes(metricsAttributes);

        ASSERT_FALSE(metrics.ValidateAgainstSchema());
    }

    TEST_F(MetricsEventTest, ValidateAgainstSchema_ValidRequiredMetricsAttributes_Success)
    {
        AZStd::vector<MetricsAttribute> metricsAttributes = GetRequiredMetricsAttributes();

        MetricsEvent metrics;
        metrics.AddAttributes(metricsAttributes);

        ASSERT_TRUE(metrics.ValidateAgainstSchema());
    }
}
