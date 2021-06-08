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

#include <Editor/Attribution/AWSCoreAttributionMetric.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace AWSCore
{
    using AttributionMetricTest = UnitTest::ScopedAllocatorSetupFixture;

    TEST_F(AttributionMetricTest, Contruction_Test)
    {
        AZStd::string timestamp = AttributionMetric::GenerateTimeStamp();
        AttributionMetric metric(timestamp);

        AZStd::string serializedMetric = AZStd::string::format(
            "{\"version\":\"1.1\",\"o3de_version\":\"\",\"platform\":\"\",\"platform_version\":\"\",\"timestamp\":\"%s\"}", timestamp.c_str());
        ASSERT_EQ(metric.SerializeToJson(), serializedMetric);
    }

    TEST_F(AttributionMetricTest, AddActiveGems)
    {
        AZStd::string timestamp = AttributionMetric::GenerateTimeStamp();
        AttributionMetric metric(timestamp);

        AZStd::string gem1 = "AWSGem1";
        AZStd::string gem2 = "AWSGem2";

        metric.AddActiveGem(gem1);
        metric.AddActiveGem(gem2);

        AZStd::string serializedMetric = AZStd::string::format(
            "{\"version\":\"1.1\",\"o3de_version\":\"\",\"platform\":\"\",\"platform_version\":\"\",\"aws_gems\":[\"%s\",\"%s\"],\"timestamp\":\"%s\"}",
            gem1.c_str(), gem2.c_str(), timestamp.c_str());

        AZStd::string actualValue = metric.SerializeToJson();
        ASSERT_EQ(actualValue, serializedMetric);
    }

} // namespace AWSCore
