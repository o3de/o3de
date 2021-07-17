/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <MetricsEventBuilder.h>
#include <MetricsEvent.h>

namespace AWSMetrics
{
    class MetricsEventBuilderTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        //! Default attributes include client id, timestamp, event id and event source.
        const int NumDefaultMetrics = 4;
        const int NumProvidedMetrics = 10;
        const AZStd::string AttrName = "name";
        const AZStd::string AttrValue = "value";
        const AZStd::string FakeClientId = "fakeClientId";

        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();

            m_metricsEventBuilder = AZStd::make_unique<MetricsEventBuilder>();
        }

        void TearDown() override
        {
            m_metricsEventBuilder.reset();
            UnitTest::ScopedAllocatorSetupFixture::TearDown();
        }

        AZStd::unique_ptr<MetricsEventBuilder> m_metricsEventBuilder;
    };

    TEST_F(MetricsEventBuilderTest, BuildMetricsEvent_DefaultAttributes_Success)
    {
        MetricsEvent metricsEvent = m_metricsEventBuilder->AddDefaultMetricsAttributes(FakeClientId).Build();

        ASSERT_EQ(metricsEvent.GetNumAttributes(), NumDefaultMetrics);
    }

    TEST_F(MetricsEventBuilderTest, BuildMetricsEvent_ProvidedAttributes_Success)
    {
        AZStd::vector<MetricsAttribute> metricsAttributes;
        for (int index = 0; index < NumProvidedMetrics; ++index)
        {
            metricsAttributes.emplace_back(MetricsAttribute(AZStd::string::format("%s%i", AttrName.c_str(), index), AttrValue));
        }

        MetricsEvent metricsEvent = m_metricsEventBuilder->AddMetricsAttributes(metricsAttributes).Build();

        // Timestamp attribute will be added during creation automatically.
        ASSERT_EQ(metricsEvent.GetNumAttributes(), NumProvidedMetrics + 1);
    }

    TEST_F(MetricsEventBuilderTest, BuildMetricsEvent_SetMetricsPriority_Success)
    {
        MetricsEvent metricsEvent = m_metricsEventBuilder->SetMetricsPriority(0).Build();

        ASSERT_EQ(metricsEvent.GetEventPriority(), 0);
    }
}
