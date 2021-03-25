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

#include <AWSMetricsConstant.h>
#include <MetricsAttribute.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace AWSMetrics
{
    class MetricsAttributeTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        const AZStd::string attrName = "name";
        const AZStd::string strValue = "value";
        const int intValue = 0;
        const double doubleValue = 0.01;
    };

    TEST_F(MetricsAttributeTest, SetAttributeName_DefaultConstructor_Success)
    {
        MetricsAttribute attribute;
        attribute.SetName(attrName.c_str());

        ASSERT_EQ(attribute.GetName(), attrName);
        ASSERT_FALSE(attribute.IsDefault());

        attribute.SetName(METRICS_ATTRIBUTE_KEY_EVENT_NAME);
        ASSERT_TRUE(attribute.IsDefault());
    }

    TEST_F(MetricsAttributeTest, SetAttributeValue_SupportedAttributeTypes_Success)
    {
        MetricsAttribute attribute;

        attribute.SetVal(strValue);
        ASSERT_EQ(AZStd::get<AZStd::string>(attribute.GetVal()), strValue);

        attribute.SetVal(intValue);
        ASSERT_EQ(AZStd::get<int>(attribute.GetVal()), intValue);

        attribute.SetVal(doubleValue);
        ASSERT_EQ(AZStd::get<double>(attribute.GetVal()), doubleValue);
    }

    TEST_F(MetricsAttributeTest, GetSizeInBytes_SupportedAttributeTypes_Success)
    {
        MetricsAttribute strAttr(attrName, strValue);
        ASSERT_EQ(strAttr.GetSizeInBytes(), sizeof(char) * (attrName.size() + strValue.size()));

        MetricsAttribute intAttr(attrName, intValue);
        ASSERT_EQ(intAttr.GetSizeInBytes(), sizeof(char) * attrName.size() + sizeof(int));

        MetricsAttribute doubleAttr(attrName, doubleValue);
        ASSERT_EQ(doubleAttr.GetSizeInBytes(), sizeof(char) * attrName.size() + sizeof(double));
    }

    TEST_F(MetricsAttributeTest, SerializeToJson_SupportedAttributeTypes_Success)
    {
        std::ostream stream(nullptr);
        AWSCore::JsonOutputStream jsonStream{ stream };
        AWSCore::JsonWriter strWriter{ jsonStream };
        AWSCore::JsonWriter intWriter{ jsonStream };
        AWSCore::JsonWriter doubleWriter{ jsonStream };

        MetricsAttribute attribute;
        attribute.SetName(attrName.c_str());

        attribute.SetVal(strValue);
        ASSERT_TRUE(attribute.SerializeToJson(strWriter));

        attribute.SetVal(intValue);
        ASSERT_TRUE(attribute.SerializeToJson(intWriter));

        attribute.SetVal(doubleValue);
        ASSERT_TRUE(attribute.SerializeToJson(doubleWriter));
    }

    TEST_F(MetricsAttributeTest, ReadFromJson_SupportedAttributeTypes_Success)
    {
        MetricsAttribute attribute;

        rapidjson::Value nameVal(rapidjson::kStringType);
        nameVal.SetString(rapidjson::StringRef(attrName.c_str()));
        rapidjson::Value strVal(rapidjson::kStringType);
        nameVal.SetString(rapidjson::StringRef(strValue.c_str()));
        rapidjson::Value intVal(rapidjson::kNumberType);
        intVal.SetInt(intValue);
        rapidjson::Value doubleVal(rapidjson::kNumberType);
        doubleVal.SetDouble(doubleValue);

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
        nameVal.SetString(rapidjson::StringRef(attrName.c_str()));
        rapidjson::Value arrayVal(rapidjson::kArrayType);

        ASSERT_FALSE(attribute.ReadFromJson(nameVal, arrayVal));
    }
}
