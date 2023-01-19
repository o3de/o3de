/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/GradientSignalTestFixtures.h>
#include <GradientSignal/Editor/EditorGradientPreviewRenderer.h>
#include <Editor/EditorConstantGradientComponent.h>

#include <AzTest/AzTest.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Math/Aabb.h>

namespace UnitTest
{
    // Extend the GradientSignalTestEnvironment to include any editor-specific component descriptors that we might need.
    class GradientSignalEditorTestEnvironment : public GradientSignalTestEnvironment
    {
    public:

        void AddGemsAndComponents() override
        {
            GradientSignalTestEnvironment::AddGemsAndComponents();

            AddComponentDescriptors({
                GradientSignal::EditorConstantGradientComponent::CreateDescriptor(),
            });
        }
    };


    struct EditorGradientSignalPreviewTestsFixture
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

        void TestPreviewImage(int imageBounds, const AZStd::vector<AZ::Vector3>& interlaceOrder)
        {
            // NOTE:  We currently only test square images.  If we want to test rectangular ones, we'd need to copy the
            // centering logic from the EditorGradientPreviewRenderer to validate that the gradient values are ending up
            // in the right pixels.  It seems a bit redundant, so the tests are currently constrained to squares.
            int imageBoundsX = imageBounds;
            int imageBoundsY = imageBounds;

            // Create a mock gradient entity and a mock entity that owns the preview widget
            auto entityMock = CreateEntity();
            auto previewOwnerEntityMock = CreateEntity();

            // Set up preview bounds.  We set them to match up 1:1 with the size of our generated mock gradient data
            // so that we can easily validate that the output preview pixels exactly match the input mock data, and
            // we can easily validate the order in which the gradient values were requested to test the interlacing
            // functionality.
            bool constrainToShape = false;
            AZ::Aabb previewBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(aznumeric_cast<float>(imageBoundsX), aznumeric_cast<float>(imageBoundsY), 0.0f));

            // Fill in our mock gradient data with values that go from 0.0f in the upper left corner down to 1.0f in the bottom right.
            AZStd::vector<float> inputData(imageBoundsX * imageBoundsY);

            for (int y = 0; y < imageBoundsY; y++)
            {
                for (int x = 0; x < imageBoundsX; x++)
                {
                    inputData.push_back(static_cast<float>(x * y) / static_cast<float>((imageBoundsY - 1) * (imageBoundsX - 1)));
                }
            }

            // Set up a gradient sampler that points to our mock entities and listens to the correct gradient EBuses
            GradientSignal::GradientSampler sampler;
            sampler.m_gradientId = entityMock->GetId();
            sampler.m_ownerEntityId = previewOwnerEntityMock->GetId();
            UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(entityMock->GetId(), inputData, imageBoundsX);
            UnitTest::MockGradientPreviewContextRequestBus mockGradientPreviewContextRequestBus(previewOwnerEntityMock->GetId(), previewBounds, constrainToShape);

            // Create an empty output preview image, with bounds set to 1:1 match with our mock gradient data.
            QImage previewImage;
            QSize imageDimensions(imageBoundsX, imageBoundsY);

            // Run the gradient preview job and wait for it to finish.
            auto updateJob = aznew GradientSignal::EditorGradientPreviewUpdateJob();
            updateJob->RefreshPreview(sampler, nullptr, imageDimensions, &previewImage);
            updateJob->Wait();

            // Verify that we got the exact image format and size that we expected.
            EXPECT_EQ(previewImage.format(), QImage::Format_Grayscale8);
            ASSERT_EQ(previewImage.size(), imageDimensions);

            // Run through the image and verify that every pixel has the value that we expected.
            AZ::u8* buffer = static_cast<AZ::u8*>(previewImage.bits());
            const uint64_t imageBytesPerLine = previewImage.bytesPerLine();
            for (int y = 0; y < imageBoundsY; y++)
            {
                for (int x = 0; x < imageBoundsX; x++)
                {
                    EXPECT_EQ(buffer[(y * imageBytesPerLine) + x], static_cast<AZ::u8>(inputData[(y * imageBoundsX) + x] * 255));
                }
            }

            // Verify that we requested the exact number of pixels in our image, no more, no less.
            EXPECT_EQ(mockGradientRequestsBus.m_positionsRequested.size(), (imageBoundsX * imageBoundsY));

            // Check to see if the values requested from the gradient were accessed in the exact interlaced order
            // that we expect.  This is an optional check, so only perform it if we passed in the expected order.
            if (!interlaceOrder.empty())
            {
                ASSERT_EQ(interlaceOrder.size(), mockGradientRequestsBus.m_positionsRequested.size());
                for (int y = 0; y < imageBoundsY; y++)
                {
                    for (int x = 0; x < imageBoundsX; x++)
                    {
                        // Verify X matches up
                        EXPECT_EQ(interlaceOrder[(y*imageBoundsX) + x].GetX(), mockGradientRequestsBus.m_positionsRequested[(y * imageBoundsX) + x].GetX());

                        // Y should be requested exactly flipped from what we would expect, since images are rendered upside-down relative to world space.
                        EXPECT_EQ((imageBoundsY - 1.0f) - interlaceOrder[(y * imageBoundsX) + x].GetY(), mockGradientRequestsBus.m_positionsRequested[(y * imageBoundsX) + x].GetY());
                    }
                }
            }

            delete updateJob;
        }
    };

    TEST_F(EditorGradientSignalPreviewTestsFixture, GradientPreviewImage_2x2_BasicFunctionality)
    {
        AZStd::vector<AZ::Vector3> interlaceOrder {
            AZ::Vector3(0.0f, 0.0f, 0.0f),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f),
            AZ::Vector3(1.0f, 1.0f, 0.0f),
        };

        TestPreviewImage(2, interlaceOrder);
    }

    TEST_F(EditorGradientSignalPreviewTestsFixture, GradientPreviewImage_4x4_BasicFunctionality)
    {
        AZStd::vector<AZ::Vector3> interlaceOrder{
            AZ::Vector3(0.0f, 0.0f, 0.0f),
            AZ::Vector3(2.0f, 0.0f, 0.0f),
            AZ::Vector3(0.0f, 2.0f, 0.0f),
            AZ::Vector3(2.0f, 2.0f, 0.0f),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            AZ::Vector3(3.0f, 0.0f, 0.0f),
            AZ::Vector3(1.0f, 2.0f, 0.0f),
            AZ::Vector3(3.0f, 2.0f, 0.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f),
            AZ::Vector3(1.0f, 1.0f, 0.0f),
            AZ::Vector3(2.0f, 1.0f, 0.0f),
            AZ::Vector3(3.0f, 1.0f, 0.0f),
            AZ::Vector3(0.0f, 3.0f, 0.0f),
            AZ::Vector3(1.0f, 3.0f, 0.0f),
            AZ::Vector3(2.0f, 3.0f, 0.0f),
            AZ::Vector3(3.0f, 3.0f, 0.0f),
        };

        TestPreviewImage(4, interlaceOrder);
    }

    TEST_F(EditorGradientSignalPreviewTestsFixture, GradientPreviewImage_1100x1100_LargeImage)
    {
        // NOTE:  we leave the interlaceOrder vector empty to skip validating the interlace pattern.
        // It's too complicated to fill in programatically, and too large to write out manually.
        AZStd::vector<AZ::Vector3> interlaceOrder;

        TestPreviewImage(1100, interlaceOrder);
    }

    TEST_F(EditorGradientSignalPreviewTestsFixture, GradientPreviewImage_DefaultsToPinningItself)
    {
        // Verify that the GradientPreviewer will automatically set itself to preview against its own entity's bounds if it
        // hasn't already been pinned to preview with a different entity.

        float shapeHalfBounds = 20.0f;

        // Create an Editor Constant Gradient Component with arbitrary parameters. We need the Editor version so that it has
        // a gradient previewer.
        auto entity = CreateTestEntity(shapeHalfBounds);
        entity->CreateComponent<GradientSignal::EditorConstantGradientComponent>();
        ActivateEntity(entity.get());

        // Verify that by default, the gradient previewer is hooked up to the entity that it exists on.
        AZ::EntityId previewEntityId;
        GradientSignal::GradientPreviewContextRequestBus::EventResult(
            previewEntityId, entity->GetId(), &GradientSignal::GradientPreviewContextRequestBus::Events::GetPreviewEntity);
        EXPECT_EQ(entity->GetId(), previewEntityId);
    }
} // namespace UnitTest

// This uses a custom test hook so that we can load LmbrCentral and use Shape components in our unit tests.
AZ_UNIT_TEST_HOOK(new UnitTest::GradientSignalEditorTestEnvironment);
