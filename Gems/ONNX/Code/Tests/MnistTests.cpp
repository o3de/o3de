/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mnist.h>

#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzTest/GemTestEnvironment.h>
#include <AzToolsFramework/Commands/PreemptiveUndoCache.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <UnitTest/ToolsTestApplication.h>
#include <QApplication>

namespace Mnist
{
    class MnistTestEnvironment : public AZ::Test::GemTestEnvironment
    {
        AZ::ComponentApplication* CreateApplicationInstance() override
        {
            // Using ToolsTestApplication to have AzFramework and AzToolsFramework components.
            return aznew UnitTest::ToolsTestApplication("Mnist");
        }

        public:
            MnistTestEnvironment() = default;
            ~MnistTestEnvironment() override = default;
    };

    class MnistFixture : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
        }

        void TearDown() override
        {
        }
    };

    TEST_F(MnistFixture, ModelAccuracyGreaterThan90PercentWithCpu)
    {
        ::ONNX::PrecomputedTimingData* timingData;
        ::ONNX::ONNXRequestBus::BroadcastResult(timingData, &::ONNX::ONNXRequestBus::Events::GetPrecomputedTimingData);

        float accuracy = (float)timingData->m_numberOfCorrectInferences / (float)timingData->m_totalNumberOfInferences;

        EXPECT_GT(accuracy, 0.9f);
    }

#ifdef ENABLE_CUDA
    TEST_F(MnistFixture, ModelAccuracyGreaterThan90PercentWithCuda)
    {
        ::ONNX::PrecomputedTimingData* timingDataCuda;
        ::ONNX::ONNXRequestBus::BroadcastResult(timingDataCuda, &::ONNX::ONNXRequestBus::Events::GetPrecomputedTimingDataCuda);

        float accuracy = (float)timingDataCuda->m_numberOfCorrectInferences / (float)timingDataCuda->m_totalNumberOfInferences;

        EXPECT_GT(accuracy, 0.9f);
    }
#endif
} // namespace Mnist

AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    QApplication app(argc, argv);
    AZ::Test::ApplyGlobalParameters(&argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({ new Mnist::MnistTestEnvironment });
    int result = RUN_ALL_TESTS();
    return result;
}
