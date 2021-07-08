/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
