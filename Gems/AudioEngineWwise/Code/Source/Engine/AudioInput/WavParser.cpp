/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AudioInput/WavParser.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    /*static*/ const AZ::u8 WavFileParser::riff_tag[4] = { 'R', 'I', 'F', 'F' };
    /*static*/ const AZ::u8 WavFileParser::wave_tag[4] = { 'W', 'A', 'V', 'E' };
    /*static*/ const AZ::u8 WavFileParser::fmt__tag[4] = { 'f', 'm', 't', ' ' };
    /*static*/ const AZ::u8 WavFileParser::data_tag[4] = { 'd', 'a', 't', 'a' };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    WavFileParser::WavFileParser()
    {
        ::memset(&m_header, 0, sizeof(m_header));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    WavFileParser::~WavFileParser()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    size_t WavFileParser::ParseHeader(AZ::IO::FileIOStream& fileStream)
    {
        if (IsHeaderValid())
        {
            // Header was already parsed then, no work needed.
            return 0;
        }

        AZ_Assert(fileStream.IsOpen(), "WavFileParser::ParseHeader - FileIOStream is not open!\n");

        // Parsers are allowed to seek into the stream if they want in order to perform their task
        // of gathering file information.  Will return the byte-offset into the file where the
        // data starts.
        fileStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        // Begin parsing, start with the RIFF + WAVE tags...
        AZ::u8* writePtr = reinterpret_cast<AZ::u8*>(&m_header);
        size_t copySize = sizeof(m_header.riff) + sizeof(m_header.wave);
        fileStream.Read(copySize, writePtr);

        if (!ValidTag(m_header.riff.tag, WavFileParser::riff_tag))
        {
            AZ_Error("WavFileParser", false, "WavFileParser::ParseHeader - Not a 'RIFF'!\n");
            return 0;
        }

        if (!ValidTag(m_header.wave, WavFileParser::wave_tag))
        {
            AZ_Error("WavFileParser", false, "WavFileParser::ParseHeader - Not a 'RIFF / WAVE'!\n");
            return 0;
        }

        writePtr += copySize;

        bool formatTagFound = false;
        bool dataTagFound = false;

        while (!dataTagFound)
        {
            // read the next tag, check what it is...
            ChunkHeader header;
            copySize = sizeof(header);
            fileStream.Read(copySize, &header);

            if (ValidTag(header.tag, WavFileParser::fmt__tag))
            {
                m_header.fmt.header = header;
                writePtr = reinterpret_cast<AZ::u8*>(&m_header.fmt);
                writePtr += sizeof(m_header.fmt.header);    // skip forward because it was already read into the temp chunkheader.
                copySize = sizeof(m_header.fmt) - sizeof(m_header.fmt.header);
                fileStream.Read(copySize, writePtr);

                formatTagFound = true;
            }
            else if (ValidTag(header.tag, WavFileParser::data_tag))
            {
                m_header.data = header;

                dataTagFound = true;
            }
            else
            {
                // Unknown tag, skip by the size specified
                // It is possible that we want to read certain tag data in the future.
                // Tools/encoders may embed extra data in various sections.
                fileStream.Seek(header.size, AZ::IO::GenericStream::ST_SEEK_CUR);
            }

            // Check for Eof (premature)...
            if (fileStream.GetCurPos() == fileStream.GetLength())
            {
                AZ_Error("WavFileParser", false, "WavFileParser::ParseHeader - Got to end of file and did not locate a 'data' chunk!\n");
                return 0;
            }
        }

        if (!ValidTag(m_header.fmt.header.tag, WavFileParser::fmt__tag))
        {
            AZ_Error("WavFileParser", false, "WavFileParser::ParseHeader - Did not find a 'fmt' tag!\n");
        }

        if (!ValidTag(m_header.data.tag, WavFileParser::data_tag))
        {
            AZ_Error("WavFileParser", false, "WavFileParser::ParseHeader - Did not find a 'data' tag!\n");
        }

#ifdef AZ_DEBUG_BUILD
        if (formatTagFound)
        {
            AZ_TracePrintf("WavFileParser", "Format: %u\n", static_cast<AZ::u32>(GetSampleType()));
            AZ_TracePrintf("WavFileParser", "Channels: %u\n", GetNumChannels());
            AZ_TracePrintf("WavFileParser", "SampleRate: %u\n", GetSampleRate());
            AZ_TracePrintf("WavFileParser", "ByteRate: %u\n", GetByteRate());
            AZ_TracePrintf("WavFileParser", "BitsPerSample: %u\n", GetBitsPerSample());
            AZ_TracePrintf("WavFileParser", "DataSize: %u\n", GetDataSize());
        }
#endif // AZ_DEBUG_BUILD

        if (dataTagFound && formatTagFound)
        {
            m_headerIsValid = true;
            return fileStream.GetCurPos();
        }
        else
        {
            return 0;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AudioInputSampleType WavFileParser::GetSampleType() const
    {
        switch (m_header.fmt.audioFormat)
        {
        case 1:
            return AudioInputSampleType::Int;
        case 3:
            return AudioInputSampleType::Float;
        default:
            return AudioInputSampleType::Unsupported;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // static
    AZ_FORCE_INLINE bool WavFileParser::ValidTag(const AZ::u8 tag[4], const AZ::u8 name[4])
    {
        return (tag[0] == name[0] && tag[1] == name[1] && tag[2] == name[2] && tag[3] == name[3]);
    }

} // namespace Audio
