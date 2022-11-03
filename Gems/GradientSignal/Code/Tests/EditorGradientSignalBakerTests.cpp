/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzTest/Utils.h>

#include <GradientSignal/Editor/EditorGradientBakerComponent.h>

// this needs to be included before OpenImageIO because of WIN32 GetObject macro conflicting with RegistrySettings::GetObject
#include <Tests/GradientSignalTestFixtures.h>

AZ_PUSH_DISABLE_WARNING(4777, "-Wunknown-warning-option")
#include <OpenImageIO/imageio.h>
AZ_POP_DISABLE_WARNING


namespace UnitTest
{
    struct EditorGradientSignalBakerTestsFixture
        : public GradientSignalTest
    {
    protected:
        AZ::JobManager* m_jobManager = nullptr;
        AZ::JobContext* m_jobContext = nullptr;

        void SetUp() override
        {
            GradientSignalTest::SetUp();

            auto globalContext = AZ::JobContext::GetGlobalContext();
            if (globalContext)
            {
                AZ_Assert(
                    globalContext->GetJobManager().GetNumWorkerThreads() >= 2,
                    "Job Manager previously started by test environment with too few threads for this test.");
            }
            else
            {
                // Set up job manager with two threads so that we can run and test the preview job logic.
                AZ::JobManagerDesc desc;
                AZ::JobManagerThreadDesc threadDesc;
                desc.m_workerThreads.push_back(threadDesc);
                desc.m_workerThreads.push_back(threadDesc);
                m_jobManager = aznew AZ::JobManager(desc);
                m_jobContext = aznew AZ::JobContext(*m_jobManager);
                AZ::JobContext::SetGlobalContext(m_jobContext);
            }
        }

        void TearDown() override
        {
            if (m_jobContext)
            {
                AZ::JobContext::SetGlobalContext(nullptr);
                delete m_jobContext;
                delete m_jobManager;
            }

            GradientSignalTest::TearDown();
        }

        void TestBakeImage(
            AZStd::string extension,
            GradientSignal::OutputFormat outputFormat,
            AZ::Vector2 outputResolution = AZ::Vector2(10.0f),
            bool useValidGradient = true,
            AZ::Aabb inputBounds = AZ::Aabb::CreateNull())
        {
            // If we are going to test against a valid gradient, create a random float value between [0,1) to use as the expected value so
            // that we can guarantee each test is executing against a unique image
            AZ::SimpleLcgRandom random;
            float expectedValue = (useValidGradient) ? random.GetRandomFloat() : 0.0f;

            // Build a constant gradient with our expected value to be used as the input to the gradient baker
            auto constantGradientEntity = BuildTestConstantGradient(10.0f, expectedValue);

            AZ::EntityId inputGradientEntityId;
            if (useValidGradient)
            {
                inputGradientEntityId = constantGradientEntity->GetId();
            }

            // Setup our gradient baker configuration as per the test inputs
            GradientSignal::GradientBakerConfig configuration;
            configuration.m_gradientSampler.m_gradientId = inputGradientEntityId;
            configuration.m_outputFormat = outputFormat;
            configuration.m_outputResolution = outputResolution;
            configuration.m_inputBounds = inputGradientEntityId;

            // Create a temporary directory that will be deleted (along with its contents) after the test is complete that will hold our
            // baked output image
            AZ::Test::ScopedAutoTempDirectory tempDir;

            // Resolve a full file path for the baked output image based on the extension we are testing inside our temporary directory
            AZStd::string outputFilename = "baked_output";
            outputFilename += extension;
            AZ::IO::Path fullPath(tempDir.Resolve(outputFilename.c_str()));

            // Create an input bounds (if one wasn't passed in, which is the default case)
            // If an input bounds was explicitly passed in, we are assuming it is for the BoundsHalfOverlap
            // which changes which pixels we are going to compare against at the end of the test
            bool compareAgainstFirstPixel = true;
            if (!inputBounds.IsValid())
            {
                inputBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(10.0f, 10.0f, 0.0f));
            }
            else
            {
                compareAgainstFirstPixel = false;
            }

            // Create the bake job and wait until it completes
            auto bakeJob = aznew GradientSignal::BakeImageJob(configuration, fullPath, inputBounds, configuration.m_inputBounds);
            bakeJob->Start();
            bakeJob->Wait();

            auto imageInput = OIIO::ImageInput::open(fullPath.c_str());
            ASSERT_NE(imageInput, nullptr);

            // Make sure the image that was loaded had no errors
            EXPECT_EQ(imageInput->has_error(), false);

            // Make sure the expected image resolution matches the resolution spec of the actual file that was baked
            const OIIO::ImageSpec& spec = imageInput->spec();
            const int imageResolutionX = aznumeric_cast<int>(configuration.m_outputResolution.GetX());
            const int imageResolutionY = aznumeric_cast<int>(configuration.m_outputResolution.GetY());
            EXPECT_EQ(spec.width, imageResolutionX);
            EXPECT_EQ(spec.height, imageResolutionY);

            // Read in the image pixels
            AZStd::vector<float> pixels(spec.width * spec.height * spec.nchannels);
            imageInput->read_image(OIIO::TypeDesc::FLOAT, pixels.data());

            // For most tests we are going to check against the first pixel we find, but for the bounds overlap
            // test case we need to instead compare against the opposite edge of the image
            int firstPixelIndex = 0;
            if (!compareAgainstFirstPixel)
            {
                firstPixelIndex = (spec.width * spec.nchannels) - 1;
            }

            // For the R8 output format, we don't have enough granularity to satisfy the default
            // float value tolerance, so we need to calculate the actual tolerance threshold
            float tolerance = AZ::Constants::Tolerance;
            if (outputFormat == GradientSignal::OutputFormat::R8)
            {
                tolerance = 1.0f / std::numeric_limits<AZ::u8>::max();
            }
            EXPECT_TRUE(AZ::IsClose(pixels[firstPixelIndex], expectedValue, tolerance));

            // For the bounds overlap test case, we need to verify the first pixel (0,0) is outside
            // the bounds so it will be 0.0f
            if (!compareAgainstFirstPixel)
            {
                EXPECT_TRUE(AZ::IsClose(pixels[0], 0.0f, tolerance));
            }

            imageInput->close();
            delete bakeJob;
        }
    };

    TEST_F(EditorGradientSignalBakerTestsFixture, InvalidInputGradient)
    {
        // An invalid input gradient should cause the output image to be entirely 0.0f values
        TestBakeImage(".png", GradientSignal::OutputFormat::R8, AZ::Vector2(10.0f), false);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BoundsHalfOverlap)
    {
        // Creating an input bounds that half overlaps our test shape will result in half being the
        // expected constant value and the other half 0.0f
        AZ::Aabb inputBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-5.0f, -5.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 0.0f));
        TestBakeImage(".png", GradientSignal::OutputFormat::R8, AZ::Vector2(10.0f), true, inputBounds);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, NonSquareOutputResolution)
    {
        // Verify we support output resolutions where the width isn't equal to the height
        TestBakeImage(".png", GradientSignal::OutputFormat::R8, AZ::Vector2(13.0f, 37.0f));
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_PNG_R8)
    {
        TestBakeImage(".png", GradientSignal::OutputFormat::R8);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_TIFF_R8)
    {
        TestBakeImage(".tiff", GradientSignal::OutputFormat::R8);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_TGA_R8)
    {
        TestBakeImage(".tga", GradientSignal::OutputFormat::R8);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_EXR_R8)
    {
        TestBakeImage(".exr", GradientSignal::OutputFormat::R8);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_PNG_R16)
    {
        TestBakeImage(".png", GradientSignal::OutputFormat::R16);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_TIFF_R16)
    {
        TestBakeImage(".tiff", GradientSignal::OutputFormat::R16);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_TGA_R16)
    {
        TestBakeImage(".tga", GradientSignal::OutputFormat::R16);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_EXR_R16)
    {
        TestBakeImage(".exr", GradientSignal::OutputFormat::R16);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_PNG_R32)
    {
        TestBakeImage(".png", GradientSignal::OutputFormat::R32);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_TIFF_R32)
    {
        TestBakeImage(".tiff", GradientSignal::OutputFormat::R32);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_TGA_R32)
    {
        TestBakeImage(".tga", GradientSignal::OutputFormat::R32);
    }

    TEST_F(EditorGradientSignalBakerTestsFixture, BakedImage_EXR_R32)
    {
        TestBakeImage(".exr", GradientSignal::OutputFormat::R32);
    }
}
