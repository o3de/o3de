/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>

namespace Audio
{
    struct WAVHeader
    {
        AZ::u8 riffTag[4];
        AZ::u32 fileSize;
        AZ::u8 waveTag[4];
        AZ::u8 fmtTag[4];
        AZ::u32 fmtSize;
        AZ::u16 audioFormat;
        AZ::u16 channels;
        AZ::u32 sampleRate;
        AZ::u32 byteRate;
        AZ::u16 blockAlign;
        AZ::u16 bitsPerSample;
        AZ::u8 dataTag[4];
        AZ::u32 dataSize;

        // defaults to 16 KHz 16 bit mono PCM format
        inline WAVHeader()
            : fileSize { 0 }
            , fmtSize { 16 }
            , audioFormat { 1 }
            , channels { 1 }
            , sampleRate { 16000 }
            , bitsPerSample { 16 }
            , byteRate { 16000 * 2 } // 16 bit is 2 bytes per sample
            , blockAlign { 2 }
            , dataSize { 0 }
        {
            memcpy(riffTag, "RIFF", 4);
            memcpy(waveTag, "WAVE", 4);
            memcpy(fmtTag, "fmt ", 4);
            memcpy(dataTag, "data", 4);
        }
    };


    class WAVUtil
    {
    public:
        WAVHeader m_wavHeader;
        
        inline WAVUtil(AZ::u32 sampleRate, AZ::u16 bitsPerSample, AZ::u16 channels, bool isFloat)
        {
            m_wavHeader.sampleRate = sampleRate;
            m_wavHeader.bitsPerSample = bitsPerSample;
            m_wavHeader.channels = channels;
            m_wavHeader.byteRate = static_cast<AZ::u32>(sampleRate * channels * (bitsPerSample / 8));
            m_wavHeader.audioFormat = isFloat ? 3 : 1;  // 1 = PCM 3 = IEEE Float
        }

        // Set the wav buffer to use for class operations. This buffer should have reserved
        // space at the beginning of the buffer in the amount of sizeof(WAVHeader) which
        // this function will write over. The remaining data should be sound data that
        // matches your format
        inline bool SetBuffer(AZ::u8* buffer, AZStd::size_t bufferSize)
        {
            if(buffer == nullptr || bufferSize <= sizeof(WAVHeader))
            {
                return false;
            }
            m_buffer = buffer;
            m_bufferSize = bufferSize;
            m_wavHeader.fileSize = bufferSize - 8;   // the 'RIFF' tag and filesize aren't counted in this.
            m_wavHeader.dataSize = bufferSize - sizeof(WAVHeader);
            AZ::u8* headerBuff = reinterpret_cast<AZ::u8*>(&m_wavHeader);
            ::memcpy(m_buffer, headerBuff, sizeof(WAVHeader));
            return true;
        }

        inline bool WriteWAVToFile(const AZStd::string& filePath)
        {
            if(m_buffer == nullptr || m_bufferSize == 0)
            {
                AZ_TracePrintf("WAVUtil", "WAV buffer invalid, unable to write file. Buffer Ptr: %d, Buffer Size: %d\n", m_buffer, m_bufferSize);
                return false;
            }
            AZ::IO::FileIOStream fileStream(filePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);
            if (fileStream.IsOpen())
            {
                [[maybe_unused]] auto bytesWritten = fileStream.Write(m_bufferSize, m_buffer);
                AZ_TracePrintf("WAVUtil", "Wrote WAV file: %s, %d bytes\n", filePath.c_str(), bytesWritten);
                return true;
            }       
            else
            {
                AZ_TracePrintf("WAVUtil", "Unable to write WAV file, can't open stream for file %s\n", filePath.c_str());
                return false;
            }
        }
    
    private:
        AZ::u8* m_buffer = nullptr;
        AZStd::size_t m_bufferSize = 0;

    };
}
