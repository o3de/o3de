/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mnist.h>
#include <ONNXSystemComponent.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>
#include <AzTest/GemTestEnvironment.h>
#include <AzToolsFramework/Commands/PreemptiveUndoCache.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <QApplication>
#include <UnitTest/ToolsTestApplication.h>

namespace Mnist
{
    class MnistTestEnvironment : public AZ::Test::GemTestEnvironment
    {
        void AddGemsAndComponents() override
        {
            AddComponentDescriptors({ ::ONNX::ONNXSystemComponent::CreateDescriptor() });

            AddRequiredComponents({ ::ONNX::ONNXSystemComponent::TYPEINFO_Uuid() });
        }

        void PreCreateApplication() override
        {
            m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
            AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
        }

        void PostDestroyApplication() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            m_fileIO.reset();
        }

    private:
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
    };

    class MnistFixture : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            AZ::SettingsRegistryImpl localRegistry;
            localRegistry.Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, AZ::Test::GetEngineRootPath());

            // Look up the path to the ONNX Gem Folder (don't assume it is in the Engine root)
            // via searching through the gem paths that are registered in the o3de manifest files
            // Adding the ONNX gem as an active gem allows the alias of @gemroot:ONNX@ to be
            // set in the fileIO
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ManifestGemsPaths(localRegistry);
            AZ::Test::AddActiveGem("ONNX", localRegistry, AZ::IO::FileIOBase::GetInstance());
        }

        void TearDown() override
        {
        }
    };

    // Tests the MNIST model against 1000 images of each digit from the dataset (10,000 total), and checks model accuracy is above 90%.
    // If CUDA is enabled, the same tests will be run using CUDA for inferencing.
    TEST_F(MnistFixture, ModelAccuracyGreaterThan90PercentWithCpu)
    {
        InferenceData cpuInferenceData = RunMnistSuite(1000, false);

        float accuracy = (float)cpuInferenceData.m_numberOfCorrectInferences / (float)cpuInferenceData.m_totalNumberOfInferences;
        EXPECT_GT(accuracy, 0.9f);
    }

#ifdef ENABLE_CUDA
    TEST_F(MnistFixture, ModelAccuracyGreaterThan90PercentWithCuda)
    {
        InferenceData gpuInferenceData = RunMnistSuite(1000, true);

        float accuracy = (float)gpuInferenceData.m_numberOfCorrectInferences / (float)gpuInferenceData.m_totalNumberOfInferences;

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
