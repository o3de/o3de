/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/typetraits/is_arithmetic.h>

#define AUDIO_ALLOCATION_ALIGNMENT 16

namespace Audio
{
    /*
     * RingBufferBase defines an interface for a generic ring buffer.
     */
    class RingBufferBase
    {
    public:
        AZ_CLASS_ALLOCATOR(RingBufferBase, AZ::SystemAllocator);

        RingBufferBase()
        {}

        RingBufferBase([[maybe_unused]] const size_t numSamples)
        {}

        virtual ~RingBufferBase()
        {}

        /**
         * Adds new data to the ringbuffer.
         * @param source Source buffer to copy from.
         * @param numFrames Number of sample frames available to copy.
         * @param numChannels Number of channels in the sample data, samples = numFrames * numChannels.
         * @return Number of sample frames copied.
         */
        virtual size_t AddData(const void* source, size_t numFrames, size_t numChannels) = 0;

        /**
         * Adds new multi-track/multi-channel data to the ringbuffer in interleaved format.
         * Not a required interface.
         * @param source Source buffer to copy from.
         * @param numFrames Number of sample frames available to copy.
         * @param numChannels Number of tracks/channels in the source data, numSamples = numFrames * numChannels.
         * @return Number of sample frames copied.
         */
        virtual size_t AddMultiTrackDataInterleaved([[maybe_unused]] const void** source, [[maybe_unused]] size_t numFrames, [[maybe_unused]] size_t numChannels) { return 0; }

        /**
         * Consumes stored data from the ringbuffer.
         * @param dest Where the data will be written to, typically an array of SampleType pointers.
         * @param numFrames Number of sample frames requested to consume.
         * @param numChannels Number of channels laid out in the dest parameter.
         * @param deinterleaveMultichannel In the case of multichannel data, if true do a deinterleaved copy into the dest array channels otherwise straight copy into dest[0].
         * @return Number of sample frames consumed.
         */
        virtual size_t ConsumeData(void** dest, size_t numFrames, size_t numChannels, bool deinterleaveMultichannel = false) = 0;

        /**
         * Zeros the ringbuffer data and resets indices.
         */
        virtual void ResetBuffer() = 0;
    };


    /*
     *   RingBuffer<T>
     *
     *       m_read ---->                m_write ---->
     *       V                           V
     *   +-------------------------------------------------+
     *   |   DATADATADATADATADATADATADATA                  |
     *   +-------------------------------------------------+
     *   ^
     *   m_buffer
     *
     *   <---------------------m_size---------------------->
     */
    template <typename SampleType, typename = AZStd::enable_if_t<AZStd::is_arithmetic<SampleType>::value>>
    class RingBuffer
        : public RingBufferBase
    {
    public:
        AZ_CLASS_ALLOCATOR(RingBuffer<SampleType>, AZ::SystemAllocator);

        static const size_t s_bytesPerSample = sizeof(SampleType);

        ///////////////////////////////////////////////////////////////////////////////////////////
        RingBuffer(size_t numSamples)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            AllocateData(numSamples);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        ~RingBuffer() override
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            DeallocateData();
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        size_t AddData(const void* source, size_t numFrames, size_t numChannels) override
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

            const size_t numSamples = numFrames * numChannels;
            if (numSamples > SamplesUnused())
            {
                // Writing this many samples will cross the read index, can't proceed.
                return 0;
            }

            const SampleType* sourceBuffer = static_cast<const SampleType*>(source);
            if (m_write + numSamples < m_size)
            {
                // write operation won't wrap the buffer
                if (sourceBuffer)
                {
                    size_t copySize = numSamples * s_bytesPerSample;
                    ::memcpy(&m_buffer[m_write], sourceBuffer, copySize);
                }
                else
                {
                    ::memset(&m_buffer[m_write], 0, numSamples * s_bytesPerSample);
                }
                m_write += numSamples;
            }
            else
            {
                // Split the copy operations to handle wrap-around
                size_t currentToEndSamples = m_size - m_write;
                size_t wraparoundSamples = numSamples - currentToEndSamples;

                if (sourceBuffer)
                {
                    ::memcpy(&m_buffer[m_write], sourceBuffer, currentToEndSamples * s_bytesPerSample);
                    ::memcpy(m_buffer, &sourceBuffer[currentToEndSamples], wraparoundSamples * s_bytesPerSample);
                }
                else
                {
                    ::memset(&m_buffer[m_write], 0, currentToEndSamples * s_bytesPerSample);
                    ::memset(m_buffer, 0, wraparoundSamples * s_bytesPerSample);
                }

                m_write = wraparoundSamples;
            }

            return numFrames;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        size_t AddMultiTrackDataInterleaved(const void** source, size_t numFrames, size_t numChannels) override
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

            const size_t numSamples = numFrames * numChannels;
            if (numSamples > SamplesUnused())
            {
                // Writing this many samples will cross the read index, can't proceed.
                // Consumption needs to occur first to free up room for input.
                return 0;
            }

            if (m_write + numSamples < m_size)
            {
                // write operation won't wrap the buffer
                const SampleType** sourceBufferChannels = reinterpret_cast<const SampleType**>(source);
                AZ_ErrorOnce("AudioRingBuffer", sourceBufferChannels, "AudioRingBuffer - Multi-track source buffers not found!\n");

                if (sourceBufferChannels)
                {
                    for (size_t channel = 0; channel < numChannels; ++channel)
                    {
                        AZ_ErrorOnce("AudioRingBuffer", sourceBufferChannels[channel], "AudioRingBufffer - Multi-track source contains a null buffer at channel %zu!\n", channel);
                        const SampleType* sourceBuffer = sourceBufferChannels[channel];

                        if (sourceBuffer)
                        {
                            for (size_t frame = 0; frame < numFrames; ++frame)
                            {
                                m_buffer[m_write + channel + (numChannels * frame)] = *sourceBuffer++;
                            }
                        }
                        else
                        {
                            for (size_t frame = 0; frame < numFrames; ++frame)
                            {
                                m_buffer[m_write + channel + (numChannels * frame)] = 0;
                            }
                        }
                    }
                }
                else
                {
                    ::memset(&m_buffer[m_write], 0, numSamples * s_bytesPerSample);
                }

                m_write += numSamples;
            }
            else
            {
                // split the copy operations to handle wrap-around
                size_t currentToEndSamples = m_size - m_write;
                size_t wraparoundSamples = numSamples - currentToEndSamples;

                const SampleType** sourceBufferChannels = reinterpret_cast<const SampleType**>(source);
                AZ_ErrorOnce("AudioRingBuffer", sourceBufferChannels, "AudioRingBuffer - Multi-track source buffers not found!\n");

                if (sourceBufferChannels)
                {
                    size_t currentToEndFrames = (currentToEndSamples) / numChannels;
                    size_t wraparoundFrames = (numFrames - currentToEndFrames);

                    for (size_t channel = 0; channel < numChannels; ++channel)
                    {
                        AZ_ErrorOnce("AudioRingBuffer", sourceBufferChannels[channel], "AudioRingBufffer - Multi-track source contains a null buffer at channel %zu!\n", channel);
                        const SampleType* sourceBuffer = sourceBufferChannels[channel];

                        if (sourceBuffer)
                        {
                            for (size_t frame = 0; frame < currentToEndFrames; ++frame)
                            {
                                m_buffer[m_write + channel + (numChannels * frame)] = *sourceBuffer++;
                            }

                            for (size_t frame = 0; frame < wraparoundFrames; ++frame)
                            {
                                m_buffer[channel + (numChannels * frame)] = *sourceBuffer++;
                            }
                        }
                        else
                        {
                            for (size_t frame = 0; frame < currentToEndFrames; ++frame)
                            {
                                m_buffer[m_write + channel + (numChannels * frame)] = 0;
                            }

                            for (size_t frame = 0; frame < wraparoundFrames; ++frame)
                            {
                                m_buffer[channel + (numChannels * frame)] = 0;
                            }
                        }
                    }
                }
                else
                {
                    ::memset(&m_buffer[m_write], 0, currentToEndSamples * s_bytesPerSample);
                    ::memset(m_buffer, 0, wraparoundSamples * s_bytesPerSample);
                }

                m_write = wraparoundSamples;
            }

            return numFrames;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        size_t ConsumeData(void** dest, size_t numFrames, size_t numChannels, bool deinterleaveMultichannel) override
        {
            if (!dest)
            {
                return 0;
            }

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            SampleType** destBuffers = reinterpret_cast<SampleType**>(dest);

            size_t numSamples = numFrames * numChannels;
            size_t samplesReady = SamplesReady();

            if (samplesReady == 0)
            {
                return 0;
            }
            else if (numSamples > samplesReady)
            {
                numSamples = samplesReady;
                numFrames = numSamples / numChannels;   // ??  could this give incorrect number ??
            }


            if (numChannels == 2 && deinterleaveMultichannel)
            {
                if (m_write > m_read)
                {
                    // do 2ch deinterleaved copy
                    for (AZ::u32 frame = 0; frame < numFrames; ++frame)
                    {
                        destBuffers[0][frame] = m_buffer[m_read++];
                        destBuffers[1][frame] = m_buffer[m_read++];
                    }
                }
                else
                {
                    size_t currentToEndFrames = (m_size - m_read) / numChannels;
                    if (currentToEndFrames > numFrames)
                    {
                        // do 2ch deinterleaved copy
                        for (AZ::u32 frame = 0; frame < numFrames; ++frame)
                        {
                            destBuffers[0][frame] = m_buffer[m_read++];
                            destBuffers[1][frame] = m_buffer[m_read++];
                        }
                    }
                    else
                    {
                        // do two partial 2ch deinterleaved copies
                        AZ::u32 frame = 0;
                        for (; frame < currentToEndFrames; ++frame)
                        {
                            destBuffers[0][frame] = m_buffer[m_read++];
                            destBuffers[1][frame] = m_buffer[m_read++];
                        }

                        m_read = 0;

                        for (; frame < numFrames; ++frame)
                        {
                            destBuffers[0][frame] = m_buffer[m_read++];
                            destBuffers[1][frame] = m_buffer[m_read++];
                        }
                    }
                }
            }
            else if (numChannels == 6 && deinterleaveMultichannel)
            {
                if (m_write > m_read)
                {
                    // do 6ch deinterleaved copy
                    for (AZ::u32 frame = 0; frame < numFrames; ++frame)
                    {
                        destBuffers[0][frame] = m_buffer[m_read++];
                        destBuffers[1][frame] = m_buffer[m_read++];
                        destBuffers[2][frame] = m_buffer[m_read++];
                        destBuffers[3][frame] = m_buffer[m_read++];
                        destBuffers[4][frame] = m_buffer[m_read++];
                        destBuffers[5][frame] = m_buffer[m_read++];
                    }
                }
                else
                {
                    size_t currentToEndFrames = (m_size - m_read) / numChannels;
                    if (currentToEndFrames > numFrames)
                    {
                        // do 6ch deinterleaved copy
                        for (AZ::u32 frame = 0; frame < numFrames; ++frame)
                        {
                            destBuffers[0][frame] = m_buffer[m_read++];
                            destBuffers[1][frame] = m_buffer[m_read++];
                            destBuffers[2][frame] = m_buffer[m_read++];
                            destBuffers[3][frame] = m_buffer[m_read++];
                            destBuffers[4][frame] = m_buffer[m_read++];
                            destBuffers[5][frame] = m_buffer[m_read++];
                        }
                    }
                    else
                    {
                        // do two partial 6ch deinterleaved copies
                        AZ::u32 frame = 0;
                        for (; frame < currentToEndFrames; ++frame)
                        {
                            destBuffers[0][frame] = m_buffer[m_read++];
                            destBuffers[1][frame] = m_buffer[m_read++];
                            destBuffers[2][frame] = m_buffer[m_read++];
                            destBuffers[3][frame] = m_buffer[m_read++];
                            destBuffers[4][frame] = m_buffer[m_read++];
                            destBuffers[5][frame] = m_buffer[m_read++];
                        }

                        m_read = 0;

                        for (; frame < numFrames; ++frame)
                        {
                            destBuffers[0][frame] = m_buffer[m_read++];
                            destBuffers[1][frame] = m_buffer[m_read++];
                            destBuffers[2][frame] = m_buffer[m_read++];
                            destBuffers[3][frame] = m_buffer[m_read++];
                            destBuffers[4][frame] = m_buffer[m_read++];
                            destBuffers[5][frame] = m_buffer[m_read++];
                        }
                    }
                }
            }
            else
            {
                // single channel or interleaved copy, can do straight memcpy's
                if (m_write > m_read)
                {
                    ::memcpy(destBuffers[0], &m_buffer[m_read], numSamples * s_bytesPerSample);
                    m_read += numSamples;
                }
                else
                {
                    size_t currentToEndSamples = m_size - m_read;
                    if (currentToEndSamples > numSamples)
                    {
                        ::memcpy(destBuffers[0], &m_buffer[m_read], numSamples * s_bytesPerSample);
                        m_read += numSamples;
                    }
                    else
                    {
                        size_t wraparoundSamples = numSamples - currentToEndSamples;
                        ::memcpy(destBuffers[0], &m_buffer[m_read], currentToEndSamples * s_bytesPerSample);
                        ::memcpy(&destBuffers[0][currentToEndSamples], m_buffer, wraparoundSamples * s_bytesPerSample);
                        m_read = wraparoundSamples;
                    }
                }
            }

            return numFrames;
        }

    protected:
        ///////////////////////////////////////////////////////////////////////////////////////////
        void ResetBuffer() override
        {
            // no lock!  should only be called inside api that has already locked the buffer.
            // 0xAA is used rather than 0 to intitialze the buffer to make debugging easier as samples often
            // lead with 0's, it's more obvious when and where data is being written
            ::memset(m_buffer, 0xAA, m_size * s_bytesPerSample);
            m_write = 0;
            m_read = 0;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        void AllocateData(size_t numSamples)
        {
            // no lock!  should only be called inside api that has already locked the buffer.
            if (m_buffer)
            {
                DeallocateData();
            }

            m_size = numSamples;
            m_buffer = static_cast<SampleType*>(azmalloc(numSamples * s_bytesPerSample, AUDIO_ALLOCATION_ALIGNMENT, AZ::SystemAllocator));
            ResetBuffer();
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        void DeallocateData()
        {
            // no lock!  should only be called inside api that has already locked the buffer.
            if (m_buffer)
            {
                azfree(m_buffer, AZ::SystemAllocator);
                m_buffer = nullptr;
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        // Returns the number of samples in the ring buffer that are ready for consumption.
        size_t SamplesReady() const
        {
            // no lock!  should only be called inside api that has already locked the buffer.
            if (m_read > m_write)
            {   // read-head needs to wrap around to catch up to write-head
                return (m_write + m_size - m_read);
            }
            else
            {   // read-head is behind write-head and won't wrap around
                return (m_write - m_read);
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        // Returns the number of samples in the ring buffer that can be filled.
        size_t SamplesUnused() const
        {
            // no lock!  should only be called inside api that has already locked the buffer.
            return m_size - SamplesReady();
        }

    private:
        SampleType* m_buffer = nullptr; // sample buffer
        size_t m_write = 0;             // write-head index into buffer
        size_t m_read = 0;              // read-head index into buffer
        size_t m_size = 0;              // total size of buffer in samples. bytes = (m_size * s_bytesPerSample)
        AZStd::mutex m_mutex;           // protects buffer access
    };

} // namespace Audio
