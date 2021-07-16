/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AudioInput/AudioInputFile.h>

namespace Audio
{
    /**
     * A RIFF format chunk header.
     */
    struct ChunkHeader
    {
        AZ::u8 tag[4];
        AZ::u32 size;
    };

    /**
     * A WAVE format "fmt" chunk.
     */
    struct FmtChunk
    {
        ChunkHeader header;
        AZ::u16 audioFormat;
        AZ::u16 numChannels;
        AZ::u32 sampleRate;
        AZ::u32 byteRate;
        AZ::u16 blockAlign;
        AZ::u16 bitsPerSample;
    };

    /**
     * A WAVE format header.
     */
    struct WavHeader
    {
        ChunkHeader riff;
        AZ::u8 wave[4];
        FmtChunk fmt;
        ChunkHeader data;

        static const size_t MinSize = 44;
    };

    static_assert(sizeof(WavHeader) == WavHeader::MinSize, "WavHeader struct size is not 44 bytes!");


    /**
     * Type of AudioFileParser for Wav File Format.
     * Parses header information from Wav files and stores it for retrieval.
     */
    class WavFileParser
        : public AudioFileParser
    {
    public:
        AUDIO_IMPL_CLASS_ALLOCATOR(WavFileParser)

        WavFileParser();
        ~WavFileParser() override;

        size_t ParseHeader(AZ::IO::FileIOStream& fileStream) override;

        bool IsHeaderValid() const override;

        AudioInputSampleType GetSampleType() const override;
        AZ::u32 GetNumChannels() const override;
        AZ::u32 GetSampleRate() const override;
        AZ::u32 GetByteRate() const override;
        AZ::u32 GetBitsPerSample() const override;
        AZ::u32 GetDataSize() const override;

    private:
        static bool ValidTag(const AZ::u8 tag[4], const AZ::u8 name[4]);

        WavHeader m_header;
        bool m_headerIsValid = false;

        static const AZ::u8 riff_tag[4];
        static const AZ::u8 wave_tag[4];
        static const AZ::u8 fmt__tag[4];
        static const AZ::u8 data_tag[4];
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_INLINE bool WavFileParser::IsHeaderValid() const
    {
        return m_headerIsValid;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_INLINE AZ::u32 WavFileParser::GetNumChannels() const
    {
        return m_header.fmt.numChannels;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_INLINE AZ::u32 WavFileParser::GetSampleRate() const
    {
        return m_header.fmt.sampleRate;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_INLINE AZ::u32 WavFileParser::GetByteRate() const
    {
        return m_header.fmt.byteRate;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_INLINE AZ::u32 WavFileParser::GetBitsPerSample() const
    {
        return m_header.fmt.bitsPerSample;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_INLINE AZ::u32 WavFileParser::GetDataSize() const
    {
        return m_header.data.size;
    }

} // namespace Audio
