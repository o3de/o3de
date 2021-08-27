/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AudioSourceManager.h>

namespace Audio
{
    /**
     * Base class for audio file parser.
     * Any supported audio file types will have a parser implementation
     * that will parse header information to extract the audio format.
     */
    class AudioFileParser
    {
    public:
        AUDIO_IMPL_CLASS_ALLOCATOR(AudioFileParser)

        AudioFileParser() = default;
        virtual ~AudioFileParser() = default;

        AudioFileParser(const AudioFileParser&) = delete;
        AudioFileParser& operator=(const AudioFileParser&) = delete;

        /**
         * Parse header from a file stream.
         * Parses header of an audio file and returns the byte-offset into the file where the audio data begins.
         * @param fileStream An opened file stream on the audio file.
         * @return Byte-offset into the file where audio data begins.
         */
        virtual size_t ParseHeader(AZ::IO::FileIOStream& fileStream) = 0;

        /**
         * Check validity of the header info.
         * This should only return true if the header was parsed and user can expect to see valid format data.
         * @return True if the header was parsed without error.
         */
        virtual bool IsHeaderValid() const = 0;

        virtual AudioInputSampleType GetSampleType() const = 0;
        virtual AZ::u32 GetNumChannels() const = 0;
        virtual AZ::u32 GetSampleRate() const = 0;
        virtual AZ::u32 GetByteRate() const = 0;
        virtual AZ::u32 GetBitsPerSample() const = 0;
        virtual AZ::u32 GetDataSize() const = 0;
    };


    /**
     * A type of AudioInputSource representing an audio file.
     * Contains audio file data, holds a pointer to the raw data and provides methods to read chunks of data at a time
     * to an output (AkAudioBuffer).
     */
    class AudioInputFile
        : public AudioInputSource
    {
    public:
        AUDIO_IMPL_CLASS_ALLOCATOR(AudioInputFile)

        AudioInputFile(const SAudioInputConfig& sourceConfig);
        ~AudioInputFile() override;

        /**
         * Load file into buffer.
         * Use an AudioFileParser if needed to parse header information, then proceed to load the audio data
         * to the internal buffer.
         * @return True upon successful load, false otherwise.
         */
        bool LoadFile();

        /**
         * Unload the file data.
         * Release the internal buffer of file data.
         */
        void UnloadFile();

        void ReadInput(const AudioStreamData& data) override;
        void WriteOutput(AkAudioBuffer* akBuffer) override;
        bool IsOk() const override;

        void OnDeactivated() override;

        /**
         * Copy data from the internal buffer to an output buffer.
         * Copies a specified number of sample frames to an output buffer.  If more frames are requested
         * than can be copied, only allowable frames are copied and number of frames that were copied is returned.
         * @param numSampleFrames Number of sample frames requested for copy.
         * @param toBuffer Output buffer to copy to.
         * @return Number of sample frames actually copied.
         */
        size_t CopyData(size_t numSampleFrames, void* toBuffer);     // frames, not bytes!

    private:
        /**
         * Resets internal bookmarking.
         * Bookmarks are used internally to keep track of where we are in the buffer during
         * chunk-copying to output.
         */
        void ResetBookmarks();

        /**
         * Checks whether data copying has reached the end of the file data.
         * @return True if end of file has been reached, false otherwise.
         */
        bool IsEof() const;

        AZStd::unique_ptr<AudioFileParser> m_parser = nullptr;

        AZ::u8* m_dataPtr = nullptr;    ///< The internal data buffer.
        size_t m_dataSize = 0;          ///< The internal data size.

        // Bookmarks
        AZ::u8* m_dataCurrentPtr = nullptr; ///< The internal bookmark pointer.
        size_t m_dataCurrentReadSize = 0;   ///< The internal bookmark indicating how much data has been read so far.
    };

} // namespace Audio
