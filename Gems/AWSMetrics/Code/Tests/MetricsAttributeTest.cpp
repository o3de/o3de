/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSMetricsConstant.h>
#include <MetricsAttribute.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace AWSMetrics
{
    class MetricsAttributeTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        const AZStd::string AttrName = "name";
        const AZStd::string StrValue = "value";
        const int IntValue = 0;
        const double DoubleValue = 0.01;
    };

    TEST_F(MetricsAttributeTest, SetAttributeName_DefaultConstructor_Success)
    {
        MetricsAttribute attribute;
        attribute.SetName(AttrName.c_str());

        ASSERT_EQ(attribute.GetName(), AttrName);
        ASSERT_FALSE(attribute.IsDefault());

        attribute.SetName(AwsMetricsAttributeKeyEventName);
        ASSERT_TRUE(attribute.IsDefault());
    }

    TEST_F(MetricsAttributeTest, SetAttributeValue_SupportedAttributeTypes_Success)
    {
        MetricsAttribute attribute;

        attribute.SetVal(StrValue);
        ASSERT_EQ(AZStd::get<AZStd::string>(attribute.GetVal()), StrValue);

        attribute.SetVal(IntValue);
        ASSERT_EQ(AZStd::get<int>(attribute.GetVal()), IntValue);

        attribute.SetVal(DoubleValue);
        ASSERT_EQ(AZStd::get<double>(attribute.GetVal()), DoubleValue);
    }

    TEST_F(MetricsAttributeTest, GetSizeInBytes_SupportedAttributeTypes_Success)
    {
        MetricsAttribute strAttr(AttrName, StrValue);
        ASSERT_EQ(strAttr.GetSizeInBytes(), sizeof(char) * (AttrName.size() + StrValue.size()));

        MetricsAttribute intAttr(AttrName, IntValue);
        ASSERT_EQ(intAttr.GetSizeInBytes(), sizeof(char) * AttrName.size() + sizeof(int));

        MetricsAttribute doubleAttr(AttrName, DoubleValue);
        ASSERT_EQ(doubleAttr.GetSizeInBytes(), sizeof(char) * AttrName.size() + sizeof(double));
    }

    TEST_F(MetricsAttributeTest, SerializeToJson_SupportedAttributeTypes_Success)
    {
        std::ostream stream(nullptr);
        AWSCore::JsonOutputStream jsonStream{ stream };
        AWSCore::JsonWriter strWriter{ jsonStream };
        AWSCore::JsonWriter intWriter{ jsonStream };
        AWSCore::JsonWriter doubleWriter{ jsonStream };

        MetricsAttribute attribute;
        attribute.SetName(AttrName.c_str());

        attribute.SetVal(StrValue);
        ASSERT_TRUE(attribute.SerializeToJson(strWriter));

        attribute.SetVal(IntValue);
        ASSERT_TRUE(attribute.SerializeToJson(intWriter));

        attribute.SetVal(DoubleValue);
        ASSERT_TRUE(attribute.SerializeToJson(doubleWriter));
    }

    TEST_F(MetricsAttributeTest, ReadFromJson_SupportedAttributeTypes_Success)
    {
        MetricsAttribute attribute;

        rapidjson::Value nameVal(rapidjson::kStringType);
        nameVal.SetString(rapidjson::StringRef(AttrName.c_str()));
        rapidjson::Value strVal(rapidjson::kStringType);
        nameVal.SetString(rapidjson::StringRef(StrValue.c_str()));
        rapidjson::Value intVal(rapidjson::kNumberType);
        intVal.SetInt(IntValue);
        rapidjson::Value doubleVal(rapidjson::kNumberType);
        doubleVal.SetDouble(DoubleValue);

        ASSERT_TRUE(attribute.ReadFromJson(nameVal, strVal));
        ASSERT_EQ(attribute.GetName(), nameVal.GetString());
        ASSERT_EQ(attribute.GetSizeInBytes(), sizeof(char) * (AZStd::string(nameVal.GetString()).size() + AZStd::string(strVal.GetString()).size()));

        ASSERT_TRUE(attribute.ReadFromJson(nameVal, intVal));
        ASSERT_EQ(attribute.GetSizeInBytes(), sizeof(char) * AZStd::string(nameVal.GetString()).size() + sizeof(int));

        ASSERT_TRUE(attribute.ReadFromJson(nameVal, doubleVal));
        ASSERT_EQ(attribute.GetSizeInBytes(), sizeof(char) * AZStd::string(nameVal.GetString()).size() + sizeof(double));
    }

    TEST_F(MetricsAttributeTest, ReadFromJson_InvalidAttributeType_Fail)
    {
        MetricsAttribute attribute;

        rapidjson::Value nameVal(rapidjson::kStringType);
        nameVal.SetString(rapidjson::StringRef(AttrName.c_str()));
        rapidjson::Value arrayVal(rapidjson::kArrayType);

        ASSERT_FALSE(attribute.ReadFromJson(nameVal, arrayVal));
    }
}
