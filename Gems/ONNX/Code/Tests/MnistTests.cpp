/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Fixture.h>
#include <Mnist.h>

namespace ONNX
{
    class MnistFixture : public Fixture
    {
    public:
        void SetUp() override
        {
            Fixture::SetUp();
        }
    };

    TEST_F(MnistFixture, ModelAccuracyGreaterThan90PercentWithCpu)
    {
        PrecomputedTimingData* timingData;
        ONNXRequestBus::BroadcastResult(timingData, &ONNXRequestBus::Events::GetPrecomputedTimingData);

        float accuracy = (float)timingData->m_numberOfCorrectInferences / (float)timingData->m_totalNumberOfInferences;

        EXPECT_GT(accuracy, 0.9f);
    }

    TEST_F(MnistFixture, ModelAccuracyGreaterThan90PercentWithCuda)
    {
        if (ENABLE_CUDA) {
            PrecomputedTimingData* timingDataCuda;
            ONNXRequestBus::BroadcastResult(timingDataCuda, &ONNXRequestBus::Events::GetPrecomputedTimingDataCuda);

            float accuracy = (float)timingDataCuda->m_numberOfCorrectInferences / (float)timingDataCuda->m_totalNumberOfInferences;

            EXPECT_GT(accuracy, 0.9f);
        }
    }
} // namespace ONNX
