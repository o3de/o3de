/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AudioInput/AudioInputFile.h>
#include <AudioInput/WavParser.h>
#include <Common_wwise.h>

#include <AzCore/IO/FileIO.h>

#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Input File
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AudioInputFile::AudioInputFile(const SAudioInputConfig& sourceConfig)
    {
        m_config = sourceConfig;

        switch (sourceConfig.m_sourceType)
        {
        case AudioInputSourceType::WavFile:
            m_parser.reset(aznew WavFileParser());
            break;
        case AudioInputSourceType::PcmFile:
            break;
        default:
            return;
        }

        LoadFile();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AudioInputFile::~AudioInputFile()
    {
        UnloadFile();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool AudioInputFile::LoadFile()
    {
        bool result = false;

        // Filename should be relative to the project assets root e.g.: 'sounds/files/my_sound.wav'
        AZ::IO::FileIOStream fileStream(m_config.m_sourceFilename.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);

        if (fileStream.IsOpen())
        {
            m_dataSize = fileStream.GetLength();

            if (m_dataSize > 0)
            {
                // Here if a parser is available, can pass the file stream forward
                // so it can parse header information.
                // It will return the number of header bytes read, that is an offset to
                // the beginning of the real signal data.
                if (m_parser)
                {
                    size_t headerBytesRead = m_parser->ParseHeader(fileStream);
                    if (headerBytesRead > 0 && m_parser->IsHeaderValid())
                    {
                        // Update the size...
                        m_dataSize = m_parser->GetDataSize();

                        // Set the format configuration obtained from the file...
                        m_config.m_bitsPerSample = m_parser->GetBitsPerSample();
                        m_config.m_numChannels = m_parser->GetNumChannels();
                        m_config.m_sampleRate = m_parser->GetSampleRate();
                        m_config.m_sampleType = m_parser->GetSampleType();
                    }
                }


                if (IsOk())
                {
                    // Allocate a new buffer to hold the data...
                    m_dataPtr = new AZ::u8[m_dataSize];

                    // Read file into internal buffer...
                    size_t bytesRead = fileStream.Read(m_dataSize, m_dataPtr);

                    ResetBookmarks();

                    // Verify we read the full amount...
                    result = (bytesRead == m_dataSize);
                }
            }

            fileStream.Close();
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputFile::UnloadFile()
    {
        if (m_dataPtr)
        {
            delete [] m_dataPtr;
            m_dataPtr = nullptr;
        }
        m_dataSize = 0;
        m_dataCurrentPtr = nullptr;
        m_dataCurrentReadSize = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputFile::ReadInput([[maybe_unused]] const AudioStreamData& data)
    {
        // Don't really need this for File-based sources, the whole file is read in the constructor.
        // However, we may need to implement this for asynchronous loading of the file (streaming).
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputFile::WriteOutput(AkAudioBuffer* akBuffer)
    {
        AZ::u16 numSampleFramesRequested = (akBuffer->MaxFrames() - akBuffer->uValidFrames);

        if (m_config.m_sampleType == AudioInputSampleType::Int)
        {
            void* outBuffer = akBuffer->GetInterleavedData();

            AZ::u16 numSampleFramesCopied = static_cast<AZ::u16>(CopyData(numSampleFramesRequested, outBuffer));

            akBuffer->uValidFrames += numSampleFramesCopied;

            akBuffer->eState = (numSampleFramesCopied > 0) ? AK_DataReady
                : (IsEof() ? AK_NoMoreData : AK_NoDataReady);
        }
        else if (m_config.m_sampleType == AudioInputSampleType::Float)
        {
            // Not Implemented yet!
            akBuffer->eState = AK_NoMoreData;

            // Implementing this for files will likely involve de-interleaving the samples.
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool AudioInputFile::IsOk() const
    {
        bool ok = (m_dataSize > 0);
        ok &= IsFormatValid();

        if (m_parser)
        {
            ok &= m_parser->IsHeaderValid();
            ok &= (m_dataSize == m_parser->GetDataSize());
        }

        return ok;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputFile::OnDeactivated()
    {
        if (m_config.m_autoUnloadFile)
        {
            UnloadFile();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    size_t AudioInputFile::CopyData(size_t numSampleFrames, void* toBuffer)
    {
        // Copies data to an output buffer.
        // Size requested is in sample frames, not bytes!
        // Number of frames actually copied is returned.  This is useful if more
        // frames were requested than can be copied.

        if (!toBuffer || !numSampleFrames)
        {
            return 0;
        }

        const size_t frameBytes = (m_config.m_numChannels * m_config.m_bitsPerSample) >> 3;  // bits --> bytes
        size_t copySize = numSampleFrames * frameBytes;

        // Check if request is larger than remaining, trim off excess.
        if (m_dataCurrentReadSize + copySize > m_dataSize)
        {
            size_t excess = (m_dataCurrentReadSize + copySize) - m_dataSize;
            copySize -= excess;
            numSampleFrames = (copySize / frameBytes);
        }

        if (copySize > 0)
        {
            ::memcpy(toBuffer, m_dataCurrentPtr, copySize);
            m_dataCurrentReadSize += copySize;
            m_dataCurrentPtr += copySize;
        }

        return numSampleFrames;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputFile::ResetBookmarks()
    {
        m_dataCurrentPtr = m_dataPtr;
        m_dataCurrentReadSize = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool AudioInputFile::IsEof() const
    {
        return (m_dataCurrentReadSize == m_dataSize);
    }

} // namespace Audio
