/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/condition_variable.h>

// Qt
#include <QImage>
#include <QSize>
#include <QTimer>

// GradientSignal
#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>
#include <GradientSignal/GradientSampler.h>

namespace GradientSignal
{
    //! EditorGradientPreviewUpdateJob offloads the creation of a gradient preview image to another thread.
    //! This is necessary for Editor responsiveness.  With complex gradient hierarchies, large previews, and/or
    //! multiple gradient previews visible at the same time (like in Landscape Canvas), it's possible for the
    //! preview generation to take multiple seconds, or even minutes in degenerate data cases.
    //!
    //! In offloading the work, we also incrementally update the preview via an adaptive interlacing scheme, similar
    //! to GIF or PNG interlacing, so that it becomes visible and usable even before the work has completed.
    //!
    //! Implementation notes:
    //! - This directly modifies the m_previewImage from a Job thread in a non-threadsafe way while it is also being
    //! used from Qt in the main thread.  This doesn't cause any issues because we synchronously cancel the job thread
    //! any time we delete or recreate m_previewImage (such as during resizing).
    //!
    //! - The interlacing scheme is loosely based on the "Adam7" algorithm, which is used in the PNG format.  Unlike Adam7,
    //! which uses a 7-pass system to operate on 8x8 interlace patterns, the code below uses an N-pass system.  Roughly speaking,
    //! each pass doubles the number of pixels drawn relative to the previous pass.  For a 256x256 image, the passes will draw
    //! 1 pixel, 1 pixel, 2 pixels, 4 pixels, 8 pixels, 16 pixels, 32 pixels, 64 pixels, 128 pixels, ..., 32768 pixels.
    //!
    //! - We only create a single job instance per gradient preview, and the Process() function of the job runs once for
    //! each time we need to refresh the preview.  It remains dormant the rest of the time.  We can't use a fire-and-forget job
    //! because we need the ability to synchronously cancel it and wait for it to be cancelled.  This requirement comes from the
    //! way we reuse data that exists in the parent Preview widget class.  We need to manage the lifetime to be exactly the same
    //! as the widget, and we can't have multiple jobs running in parallel that modify the same widget.  (If we use fire-and-forget,
    //! even if we cancel asynchronously, it would be easy to start a new one before the old one finishes)

    class EditorGradientPreviewUpdateJob
        : public AZ::Job
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorGradientPreviewUpdateJob, AZ::ThreadPoolAllocator);

        using SampleFilterFunc = AZStd::function<float(float, const GradientSampleParams&)>;

        EditorGradientPreviewUpdateJob(AZ::JobContext* context = nullptr) :
            Job(false, context)
        {
        }

        virtual ~EditorGradientPreviewUpdateJob()
        {
            // Make sure we don't have anything running on another thread before destroying
            // the job instance itself.
            CancelAndWait();
        }

        bool CancelAndWait()
        {
            // Return whether or not this actually cancelled a job that had started, or if this job was already idle
            bool jobHadStarted = m_started;

            // To cancel, we start by notifying the Process() loop that it should cancel itself on the next iteration if
            // it's currently running.  (Note that this is an atomic bool)
            m_shouldCancel = true;

            // Then we synchronously block until the job has completed.
            Wait();

            return jobHadStarted;
        }

        void Wait()
        {
            // Jobs don't inherently have a way to block on cancellation / completion, so we need to implement it
            // ourselves.

            // If we've already started the job, block on a condition variable that gets notified at
            // the end of the Process() function.
            AZStd::unique_lock<decltype(m_previewMutex)> lock(m_previewMutex);
            if (m_started)
            {
                m_refreshFinishedNotify.wait(lock, [this] { return m_started == false; });
            }

            // Regardless of whether or not we were running, we need to reset the internal Job class status
            // and clear our cancel flag.
            Reset(true);
            m_shouldCancel = false;
        }

        //! This enables our widget to know whenever the preview image has changed.
        //! We also clear the refreshUI flag here on polling, so that we only detect changes since the last time
        //! we asked.  We don't rely on watching for the thread to be running, since it's possible for the thread
        //! to run and finish before we ever poll for the first time.
        bool ShouldRefreshUI()
        {
            return m_refreshUI.exchange(false);
        }

        //! Perform any main-thread and one-time setup needed to refresh the preview, then kick off the job.
        void RefreshPreview(GradientSampler sampler, SampleFilterFunc filterFunc, QSize imageResolution, QImage* previewImage)
        {
            // Make sure any previous run is cancelled and fully stopped before we modify any parameters below.
            // In particular, if we allocate / reallocate m_previewImage here while a job is running, we'll access
            // invalid memory.
            CancelAndWait();

            // No matter what, we'll want to at least refresh to get an all-black image.
            m_refreshUI = true;

            // Save off a copy of the parameters that we'll use to render the preview.  This way we don't have to worry
            // about them changing while we're running.
            m_sampler = sampler;
            m_filterFunc = filterFunc;
            m_imageResolution = imageResolution;

            // This is a direct pointer to the QImage used by the Preview widget.  We allocate it here, and write pixels
            // into it from the job.  The Preview widget (and Qt) read from it on the main thread to display even while we're
            // running.
            m_previewImage = previewImage;

            // If our image size has changed, resize our buffers.
            if (m_previewImage->size() != imageResolution)
            {
                *m_previewImage = QImage(imageResolution, QImage::Format_Grayscale8);
            }

            // Initialize it with all black.
            m_previewImage->fill(QColor(0, 0, 0));

            // No valid gradient, so all-black is all we need.  Done!
            if (!sampler.m_gradientId.IsValid())
            {
                return;
            }

            // Get preview image settings from the owning entity.

            m_constrainToShape = false;
            GradientPreviewContextRequestBus::EventResult(m_constrainToShape, sampler.m_ownerEntityId, &GradientPreviewContextRequestBus::Events::GetConstrainToShape);

            m_previewBounds = AZ::Aabb::CreateNull();
            GradientPreviewContextRequestBus::EventResult(m_previewBounds, sampler.m_ownerEntityId, &GradientPreviewContextRequestBus::Events::GetPreviewBounds);

            GradientPreviewContextRequestBus::EventResult(m_previewEntityId, sampler.m_ownerEntityId, &GradientPreviewContextRequestBus::Events::GetPreviewEntity);

            m_constrainToShape = m_constrainToShape && m_previewEntityId.IsValid();

            // If the preview bounds aren't valid, something went wrong (invalid IDs?), so don't draw anything more.
            // The preview bounds are the world-space coordinates that we'll use to sample our gradient.
            if (!m_previewBounds.IsValid())
            {
                return;
            }

            const AZ::Vector3 previewBoundsCenter = m_previewBounds.GetCenter();
            const AZ::Vector3 previewBoundsExtentsOld = m_previewBounds.GetExtents();
            m_previewBounds = AZ::Aabb::CreateCenterRadius(previewBoundsCenter, AZ::GetMax(previewBoundsExtentsOld.GetX(), previewBoundsExtentsOld.GetY()) / 2.0f);
            m_previewBoundsStart = AZ::Vector3(m_previewBounds.GetMin().GetX(), m_previewBounds.GetMin().GetY(), previewBoundsCenter.GetZ());

            const AZ::Vector3 previewBoundsExtents = m_previewBounds.GetExtents();
            const float previewBoundsExtentsX = previewBoundsExtents.GetX();
            const float previewBoundsExtentsY = previewBoundsExtents.GetY();

            // Get the actual resolution of our preview image.  Note that this might be non-square, depending on how the window is sized.
            const uint64_t imageResolutionX = imageResolution.width();
            const uint64_t imageResolutionY = imageResolution.height();

            // Get the largest square size that fits into our window bounds.
            m_imageBoundsX = AZStd::min(imageResolutionX, imageResolutionY);
            m_imageBoundsY = AZStd::min(imageResolutionX, imageResolutionY);

            // Get how many pixels we need to offset in x and y to center our square in the window.  Because we've made our square
            // as large as possible, one of these two values should always be 0.  
            // i.e. we'll end up with black bars on the sides or on top, but it should never be both.
            m_centeringOffsetX = (imageResolutionX - m_imageBoundsX) / 2;
            m_centeringOffsetY = (imageResolutionY - m_imageBoundsY) / 2;

            // When sampling the gradient, we can choose to either do it at the corners of each texel area we're sampling, or at the center.
            // They're both correct choices in different ways.  We're currently choosing to do the corners, which makes scaledTexelOffset = 0,
            // but the math is here to make it easy to change later if we ever decide sampling from the center provides a more intuitive preview.
            constexpr float texelOffset = 0.0f;  // Use 0.5f to sample from the center of the texel.
            m_scaledTexelOffset = AZ::Vector3(texelOffset * previewBoundsExtentsX / static_cast<float>(m_imageBoundsX),
                texelOffset* previewBoundsExtentsY / static_cast<float>(m_imageBoundsY), 0.0f);

            // Scale from our preview image size space (ex: 256 pixels) to our preview bounds space (ex: 16 meters)
            m_pixelToBoundsScale = AZ::Vector3(previewBoundsExtentsX / static_cast<float>(m_imageBoundsX),
                previewBoundsExtentsY / static_cast<float>(m_imageBoundsY), 0.0f);

            // Start of interlacing support:  For our interlacing algorithm to work, we need to work on images of powers of two.
            // Rather than actually allocate an image of that size, we simply find the smallest power of two that contains the image,
            // and then skip any pixels that fall outside the image when running through our per-pixel loop in Process() below.
            // We also calculate the number of interlacing passes that we need here.  For the algorithm to work, we always need to have
            // an odd number of interlacing passes.  We're keeping track of the *last* pass number, which is even, instead of number of passes,
            // just for calculation convenience later.
            m_imageBoundsPowerOfTwo = 1;
            m_finalInterlacingPass = 0;
            while (m_imageBoundsX > m_imageBoundsPowerOfTwo)
            {
                m_imageBoundsPowerOfTwo *= 2;
                m_finalInterlacingPass += 2;
            }

            // If you ever want to make this use a smaller number of passes, set this value to any even number.
            // A value of 0 is the same as "non-interlaced", 6 would exactly use the Adam7 algorithm, etc.
            // It's currently clamping to 30 as a somewhat aribtrary choice.
            const int64_t maxFinalInterlacingPass = 30;
            m_finalInterlacingPass = AZ::GetMin(m_finalInterlacingPass, maxFinalInterlacingPass);

            // Finally, lock our mutex, modify our status variables, and start the Job.
            {
                AZStd::lock_guard<decltype(m_previewMutex)> lock(m_previewMutex);
                m_shouldCancel = false;
                m_started = true;
                Start();
            }
        }

        //! Process runs exactly once for each time Start() is called on a Job, and processes on a Job worker thread.
        void Process() override
        {
            AZ_PROFILE_FUNCTION(Entity);

            // Guard against the case that we're trying to cancel even before we've started to run.
            if (!m_shouldCancel)
            {
                AZ_Assert(m_sampler.m_gradientId.IsValid() && m_previewBounds.IsValid(), "Invalid gradient settings.");

                AZ::u8* buffer = static_cast<AZ::u8*>(m_previewImage->bits());

                // This is the "striding value".  When walking directly through our preview image bits() buffer, there might be
                // extra pad bytes for each line due to alignment.  We use this to make sure we start writing each line at the right byte offset.
                const int64_t imageBytesPerLine = m_previewImage->bytesPerLine();

                // Keep track of the total number of pixels that we intend to process. For easy interlacing calculations, we always use
                // square power-of-two conceptual images, but we'll skip any pixels that fall outside of our actual image bounds.
                const int64_t totalPixels = (m_imageBoundsPowerOfTwo * m_imageBoundsPowerOfTwo);

                // Preallocate buffers for our gradient lookup positions, our gradient output values, and the corresponding pixel buffer
                // index to store the value into. These allow us to fetch gradient values in bulk, which is much faster than fetching them
                // individually. The max size we'll need is for our last interacing pass which requests 50% of our total pixels (as
                // described further below), so that's what we will preallocate.
                AZStd::vector<AZ::Vector3> gradientLookupPositions(totalPixels / 2);
                AZStd::vector<float> gradientValues(totalPixels / 2);
                AZStd::vector<size_t> pixelBufferIndex(totalPixels / 2);

                // The following loop uses a variant of the Adam7 interlacing algorithm that's been generalized to work for N passes,
                // instead of exactly 7 passes. The first two passes fill in 1 pixel each, and then each subsequent pass doubles the
                // number of pixels it fills in, until the last pass fills in 50%.
                // Note that m_finalInteracingPass contains the value of the final pass to process, not the total number of passes.
                // On each pass, we'll also early-out if the main thread requested a cancellation.
                for (int64_t curPass = 0; (!m_shouldCancel) && (curPass <= m_finalInterlacingPass); curPass++)
                {
                    gradientLookupPositions.clear();
                    pixelBufferIndex.clear();
                    gradientValues.clear();

                    // The general interlace formulas need a multiplier and an offset for x and y to apply to each relative pixel index.
                    //
                    // The first 3 passes are a little different than the others because they establish the base pattern:
                    // 1 . . . 2 . . .
                    // . . . . . . . .
                    // . . . . . . . .
                    // . . . . . . . . 
                    // 3 . . . 3 . . .
                    // . . . . . . . .
                    // . . . . . . . .
                    // . . . . . . . .
                    //
                    // Every 2 passes from then on do the same thing, with shrinking grids. One pass fills in the grid X midpoints on the
                    // lines that were already processed, and the second pass fills in all the equivalent points on the Y grid midpoints
                    // x . 4 . x . 4 .
                    // . . . . . . . .
                    // 5 . 5 . 5 . 5 .
                    // . . . . . . . .
                    // x . 4 . x . 4 .
                    // . . . . . . . .
                    // 5 . 5 . 5 . 5 .
                    // . . . . . . . .
                    //
                    // x 6 x 6 x 6 x 6
                    // 7 7 7 7 7 7 7 7
                    // x 6 x 6 x 6 x 6
                    // 7 7 7 7 7 7 7 7
                    // x 6 x 6 x 6 x 6
                    // 7 7 7 7 7 7 7 7
                    // x 6 x 6 x 6 x 6
                    // 7 7 7 7 7 7 7 7
                    // 
                    // The total number of pixels processed per pass starts at 1 pixel each for the first two passes, then doubles per
                    // pass till we reach 50% in the last pass, since all the other passes before it will have covered the other 50%.
                    // Ex: 7 passes will do N/64, N/64, N/32, N/16, N/8, N/4, N/2 pixels per pass.


                    // For X, we want our starting pixel offset to alternate between 0 and a decreasing power of 2 on every pass, and
                    // our stride to decrease by a power of 2 every two passes, ending with an offset of 0 and a stride of 1 on the last
                    // pass.
                    const int64_t xOffsetShifter = AZ::GetMin(m_finalInterlacingPass - curPass, m_finalInterlacingPass - 1);
                    const int64_t xPixelOffset = (curPass % 2) * (1LL << (xOffsetShifter / 2));
                    const int64_t xPixelStride = 1LL << ((xOffsetShifter + 1) / 2);

                    // For Y, we want our starting pixel offset and our stride to behave the same as X, except that we hold our starting
                    // offset and stride for one additional pass which is what causes the first 3 passes to behave differently than the
                    // rest. The pass offset between X and Y is also what causes the pattern to keep filling in pixels and lines that
                    // haven't already been processed.
                    const int64_t laggingPass = AZ::GetMax<int64_t>(curPass - 1, 0);
                    const int64_t yOffsetShifter = AZ::GetMin(m_finalInterlacingPass - laggingPass, m_finalInterlacingPass - 1);
                    const int64_t yPixelOffset = (laggingPass % 2) * (1LL << (yOffsetShifter / 2));
                    const int64_t yPixelStride = 1LL << ((yOffsetShifter + 1) / 2);

                    // First, we loop and fill in all the gradientLookupPositions and pixelBufferIndex values for any pixels that don't
                    // get culled out. We're using a power of two for calculating our interlacing offsets and strides, but we don't need
                    // to actually process any of those pixels that fall outside our image bounds, so we end our loops at the bounds.
                    for (int64_t y = yPixelOffset; y < m_imageBoundsY; y += yPixelStride)
                    {
                        for (int64_t x = xPixelOffset; x < m_imageBoundsX; x += xPixelStride)
                        {
                            // Map the pixel coordinate back into world coordinates for the shape and gradient queries. Note that we
                            // invert world y to match the world axis.  (We use "imageBoundsY- 1" to invert because our loop doesn't go all
                            // the way to imageBoundsY)
                            AZ::Vector3 uvw(static_cast<float>(x), static_cast<float>((m_imageBoundsY - 1) - y), 0.0f);
                            AZ::Vector3 position = m_previewBoundsStart + (uvw * m_pixelToBoundsScale) + m_scaledTexelOffset;

                            // If our preview is only drawing what appears inside the given shape, check to see if the pixel should be
                            // drawn.
                            bool inBounds = true;
                            if (m_constrainToShape)
                            {
                                LmbrCentral::ShapeComponentRequestsBus::EventResult(
                                    inBounds, m_previewEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, position);
                            }

                            // If we're drawing this pixel, push it into our buffer of lookup positions.
                            if (inBounds)
                            {
                                gradientLookupPositions.emplace_back(position);
                                pixelBufferIndex.emplace_back(((m_centeringOffsetY + y) * imageBytesPerLine) + (m_centeringOffsetX + x));
                            }
                        }
                    }

                    // Resize our output buffer to match our input buffer and query for all the gradient values at once.
                    gradientValues.resize(gradientLookupPositions.size());
                    m_sampler.GetValues(gradientLookupPositions, gradientValues);

                    // For each output value, run it through a filter if we were given one, then store it in the pixel buffer.
                    for (size_t index = 0; index < gradientLookupPositions.size(); index++)
                    {
                        float sample = gradientValues[index];

                        if (m_filterFunc)
                        {
                            GradientSampleParams sampleParams;
                            sampleParams.m_position = gradientLookupPositions[index];
                            sample = m_filterFunc(sample, sampleParams);
                        }

                        buffer[pixelBufferIndex[index]] = static_cast<AZ::u8>(sample * 255);

                        // Notify the main thread via atomic bool that the image has changed by at least one pixel.
                        m_refreshUI = true;
                    }
                }
            }

            // Finally, we're done updating, so notify the main thread safely that we've finished.  This is how we're able to block
            // and verify that the job completed before changing any parameters, restarting the job, or destroying ourselves.
            {
                AZStd::lock_guard<decltype(m_previewMutex)> lock(m_previewMutex);
                m_shouldCancel = false;
                m_started = false;
                m_refreshFinishedNotify.notify_all();
            }
        }
    private:
        // Local copies of preview image info 
        GradientSampler m_sampler;
        SampleFilterFunc m_filterFunc;
        QSize m_imageResolution;

        // Pointer that points directly to the preview image owned by EditorGradientPreviewRenderer
        QImage* m_previewImage = nullptr;

        // Preview image settings
        bool m_constrainToShape = false;
        AZ::Aabb m_previewBounds = AZ::Aabb::CreateNull();
        AZ::EntityId m_previewEntityId;

        // Values calculated during preview setup that we'll use during processing
        int64_t m_imageBoundsX = 0;
        int64_t m_imageBoundsY = 0;
        int64_t m_centeringOffsetX = 0;
        int64_t m_centeringOffsetY = 0;
        AZ::Vector3 m_previewBoundsStart;
        AZ::Vector3 m_pixelToBoundsScale;
        AZ::Vector3 m_scaledTexelOffset;
        int64_t m_imageBoundsPowerOfTwo = 1;
        int64_t m_finalInterlacingPass = 0;

        // Communication / synchronization mechanisms between the different threads
        AZStd::mutex m_previewMutex;
        AZStd::atomic_bool m_started = false;
        AZStd::atomic_bool m_shouldCancel = false;
        AZStd::atomic_bool m_refreshUI = false;
        AZStd::condition_variable m_refreshFinishedNotify;
    };

    class EditorGradientPreviewRenderer
        : private AZ::TickBus::Handler
    {
    public:
        using SampleFilterFunc = AZStd::function<float(float, const GradientSampleParams&)>;

        EditorGradientPreviewRenderer()
        {
            m_updateJob = aznew EditorGradientPreviewUpdateJob();
            AZ::TickBus::Handler::BusConnect();
        }

        virtual ~EditorGradientPreviewRenderer()
        {
            AZ::TickBus::Handler::BusDisconnect();
            delete m_updateJob;
        }

        void SetGradientSampler(const GradientSampler& sampler)
        {
            m_sampler = sampler;
            QueueUpdate();
        }

        void SetGradientSampleFilter(SampleFilterFunc filterFunc)
        {
            m_filterFunc = filterFunc;
            QueueUpdate();
        }

        // AZ::TickBus
        void OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time) override
        {
            if (m_refreshUpdateJob)
            {
                m_updateJob->RefreshPreview(m_sampler, m_filterFunc, GetPreviewSize(), &m_previewImage);
                m_refreshUpdateJob = false;
            }

            if (m_updateJob->ShouldRefreshUI())
            {
                OnUpdate();
            }
        }

        void QueueUpdate()
        {
            // Queue the refresh until the next tick.  Not strictly necessary, but between separate calls to
            // SetGradientSample, SetGradientSampleFilter, and the multiple times Qt can cause a widget size
            // change, we can avoid a lot of false starts/cancels with our update job by waiting till the next tick.
            m_refreshUpdateJob = true;
        }

        bool OnCancelRefresh()
        {
            // When cancelling a refresh, we cancel both the current job and any pending refreshes.
            m_refreshUpdateJob = false;
            return m_updateJob->CancelAndWait();
        }

    protected:
        /**
         * Since this base class is shared between a QWidget and a QGraphicsItem, we need to abstract
         * the actual update() call so that they can invoke the proper one
         */
        virtual void OnUpdate() = 0;

        /**
         * Same as above, we need an abstract way to retrieve the size of the actual preview image from the QGraphicsItem
         */
        virtual QSize GetPreviewSize() const = 0;

        GradientSampler m_sampler;
        SampleFilterFunc m_filterFunc;
        QImage m_previewImage;
        bool m_refreshUpdateJob = false;

        EditorGradientPreviewUpdateJob* m_updateJob = nullptr;
    };
} //namespace GradientSignal

