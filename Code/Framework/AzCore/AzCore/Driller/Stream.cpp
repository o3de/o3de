/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Driller/Stream.h>

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>

#include <AzCore/std/time.h>

#if !defined(AZCORE_EXCLUDE_ZLIB)
#   define AZ_FILE_STREAM_COMPRESSION
#endif // AZCORE_EXCLUDE_ZLIB

#if defined(AZ_FILE_STREAM_COMPRESSION)
#   include <AzCore/Compression/Compression.h>
#endif // AZ_FILE_STREAM_COMPRESSION

namespace AZ
{
    namespace Debug
    {
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Driller output stream
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        void DrillerOutputStream::Write(u32 name, const AZ::Vector3& v)
        {
            float data[4];
            unsigned int dataSize = 3 * sizeof(float);
            v.StoreToFloat4(data);
            StreamEntry de;
            de.name = name;
            de.sizeAndFlags = dataSize;
            WriteBinary(de);
            WriteBinary(data, dataSize);
        }
        void DrillerOutputStream::Write(u32 name, const AZ::Vector4& v)
        {
            float data[4];
            unsigned int dataSize = 4 * sizeof(float);
            v.StoreToFloat4(data);
            StreamEntry de;
            de.name = name;
            de.sizeAndFlags = dataSize;
            WriteBinary(de);
            WriteBinary(data, dataSize);
        }
        void DrillerOutputStream::Write(u32 name, const AZ::Aabb& aabb)
        {
            float data[7];
            unsigned int dataSize = 6 * sizeof(float);
            aabb.GetMin().StoreToFloat4(data);
            aabb.GetMax().StoreToFloat4(&data[3]);
            StreamEntry de;
            de.name = name;
            de.sizeAndFlags = dataSize;
            WriteBinary(de);
            WriteBinary(data, dataSize);
        }
        void DrillerOutputStream::Write(u32 name, const AZ::Obb& obb)
        {
            float data[10];
            unsigned int dataSize = 10 * sizeof(float); // position (Vector3), rotation (Quaternion) and halfLengths (Vector3)
            obb.GetPosition().StoreToFloat3(data);
            obb.GetRotation().StoreToFloat4(&data[3]);
            obb.GetHalfLengths().StoreToFloat3(&data[7]);
            StreamEntry de;
            de.name = name;
            de.sizeAndFlags = dataSize;
            WriteBinary(de);
            WriteBinary(data, dataSize);
        }
        void DrillerOutputStream::Write(u32 name, const AZ::Transform& tm)
        {
            float data[12];
            unsigned int dataSize = 12 * sizeof(float);
            const Matrix3x4 matrix3x4 = Matrix3x4::CreateFromTransform(tm);
            matrix3x4.StoreToRowMajorFloat12(data);
            StreamEntry de;
            de.name = name;
            de.sizeAndFlags = dataSize;
            WriteBinary(de);
            WriteBinary(data, dataSize);
        }
        void DrillerOutputStream::Write(u32 name, const AZ::Matrix3x3& tm)
        {
            float data[9];
            unsigned int dataSize = 9 * sizeof(float);
            tm.StoreToRowMajorFloat9(data);
            StreamEntry de;
            de.name = name;
            de.sizeAndFlags = dataSize;
            WriteBinary(de);
            WriteBinary(data, dataSize);
        }
        void DrillerOutputStream::Write(u32 name, const AZ::Matrix4x4& tm)
        {
            float data[16];
            unsigned int dataSize = 16 * sizeof(float);
            tm.StoreToRowMajorFloat16(data);
            StreamEntry de;
            de.name = name;
            de.sizeAndFlags = dataSize;
            WriteBinary(de);
            WriteBinary(data, dataSize);
        }
        void DrillerOutputStream::Write(u32 name, const AZ::Quaternion& tm)
        {
            float data[4];
            unsigned int dataSize = 4 * sizeof(float);
            tm.StoreToFloat4(data);
            StreamEntry de;
            de.name = name;
            de.sizeAndFlags = dataSize;
            WriteBinary(de);
            WriteBinary(data, dataSize);
        }
        void DrillerOutputStream::Write(u32 name, const AZ::Plane& plane)
        {
            Write(name, plane.GetPlaneEquationCoefficients());
        }
        void DrillerOutputStream::WriteHeader()
        {
            StreamHeader sh; // StreamHeader should be endianess independent.
            WriteBinary(&sh, sizeof(sh));
        }

        void DrillerOutputStream::WriteTimeUTC(u32 name)
        {
            AZStd::sys_time_t now = AZStd::GetTimeUTCMilliSecond();
            Write(name, now);
        }

        void DrillerOutputStream::WriteTimeMicrosecond(u32 name)
        {
            AZStd::sys_time_t now = AZStd::GetTimeNowMicroSecond();
            Write(name, now);
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Driller Input Stream
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        bool DrillerInputStream::ReadHeader()
        {
            DrillerOutputStream::StreamHeader sh; // StreamHeader should be endianess independent.
            unsigned int numRead = ReadBinary(&sh, sizeof(sh));
            (void)numRead;
            AZ_Error("IO", numRead == sizeof(sh), "We should have atleast %d bytes in the stream to read the header!", sizeof(sh));
            if (numRead != sizeof(sh))
            {
                return false;
            }
            m_isEndianSwap = AZ::IsBigEndian(static_cast<AZ::PlatformID>(sh.platform)) != AZ::IsBigEndian(AZ::g_currentPlatform);
            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Driller file stream
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //=========================================================================
        // DrillerOutputFileStream::DrillerOutputFileStream
        // [3/23/2011]
        //=========================================================================
        DrillerOutputFileStream::DrillerOutputFileStream()
        {
#if defined(AZ_FILE_STREAM_COMPRESSION)
            m_zlib = azcreate(ZLib, (&AllocatorInstance<OSAllocator>::GetAllocator()), OSAllocator);
            m_zlib->StartCompressor(2);
#endif
        }

        //=========================================================================
        // DrillerOutputFileStream::~DrillerOutputFileStream
        // [3/23/2011]
        //=========================================================================
        DrillerOutputFileStream::~DrillerOutputFileStream()
        {
#if defined(AZ_FILE_STREAM_COMPRESSION)
            azdestroy(m_zlib, OSAllocator);
#endif
        }

        //=========================================================================
        // DrillerOutputFileStream::Open
        // [3/23/2011]
        //=========================================================================
        bool DrillerOutputFileStream::Open(const char* fileName, int mode, int platformFlags)
        {
            if (IO::SystemFile::Open(fileName, mode, platformFlags))
            {
                m_dataBuffer.reserve(100 * 1024);
#if defined(AZ_FILE_STREAM_COMPRESSION)
                //              // Enable optional: encode the file in the same format as the streamer so they are interchangeable
                //              IO::CompressorHeader ch;
                //              ch.SetAZCS();
                //              ch.m_compressorId = IO::CompressorZLib::TypeId();
                //              ch.m_uncompressedSize = 0; // will be updated later
                //              AZStd::endian_swap(ch.m_compressorId);
                //              AZStd::endian_swap(ch.m_uncompressedSize);
                //              IO::SystemFile::Write(&ch,sizeof(ch));
                //              IO::CompressorZLibHeader zlibHdr;
                //              zlibHdr.m_numSeekPoints = 0;
                //              IO::SystemFile::Write(&zlibHdr,sizeof(zlibHdr));
#endif
                return true;
            }
            return false;
        }

        //=========================================================================
        // DrillerOutputFileStream::Close
        // [3/23/2011]
        //=========================================================================
        void DrillerOutputFileStream::Close()
        {
            unsigned int dataSizeInBuffer = static_cast<unsigned int>(m_dataBuffer.size());
            {
#if defined(AZ_FILE_STREAM_COMPRESSION)
                unsigned int minCompressBufferSize = m_zlib->GetMinCompressedBufferSize(dataSizeInBuffer);
                if (m_compressionBuffer.size() < minCompressBufferSize) // grow compression buffer if needed
                {
                    m_compressionBuffer.clear();
                    m_compressionBuffer.resize(minCompressBufferSize);
                }
                unsigned int compressedSize;
                do
                {
                    compressedSize = m_zlib->Compress(m_dataBuffer.data(), dataSizeInBuffer, m_compressionBuffer.data(), (unsigned)m_compressionBuffer.size(), ZLib::FT_FINISH);
                    if (compressedSize)
                    {
                        IO::SystemFile::Write(m_compressionBuffer.data(), compressedSize);
                    }
                } while (compressedSize > 0);
                m_zlib->ResetCompressor();
#else
                if (dataSizeInBuffer)
                {
                    IO::SystemFile::Write(m_dataBuffer.data(), m_dataBuffer.size());
                }
#endif
                m_dataBuffer.clear();
            }
            IO::SystemFile::Close();
        }
        //=========================================================================
        // DrillerOutputFileStream::WriteBinary
        // [3/23/2011]
        //=========================================================================
        void DrillerOutputFileStream::WriteBinary(const void* data, unsigned int dataSize)
        {
            size_t dataSizeInBuffer = m_dataBuffer.size();
            if (dataSizeInBuffer + dataSize > m_dataBuffer.capacity())
            {
                if (dataSizeInBuffer > 0)
                {
#if defined(AZ_FILE_STREAM_COMPRESSION)
                    // we need to flush the data
                    unsigned int dataToCompress = static_cast<unsigned int>(dataSizeInBuffer);
                    unsigned int minCompressBufferSize = m_zlib->GetMinCompressedBufferSize(dataToCompress);
                    if (m_compressionBuffer.size() < minCompressBufferSize) // grow compression buffer if needed
                    {
                        m_compressionBuffer.clear();
                        m_compressionBuffer.resize(minCompressBufferSize);
                    }
                    while (dataToCompress > 0)
                    {
                        unsigned int compressedSize = m_zlib->Compress(m_dataBuffer.data(), dataToCompress, m_compressionBuffer.data(), (unsigned)m_compressionBuffer.size());
                        if (compressedSize)
                        {
                            IO::SystemFile::Write(m_compressionBuffer.data(), compressedSize);
                        }
                    }
#else
                    IO::SystemFile::Write(m_dataBuffer.data(), m_dataBuffer.size());
#endif
                    m_dataBuffer.clear();
                }
            }
            m_dataBuffer.insert(m_dataBuffer.end(), reinterpret_cast<const unsigned char*>(data), reinterpret_cast<const unsigned char*>(data) + dataSize);
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Driller file input stream
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //=========================================================================
        // DrillerInputFileStream::DrillerInputFileStream
        // [3/23/2011]
        //=========================================================================
        DrillerInputFileStream::DrillerInputFileStream()
        {
#if defined(AZ_FILE_STREAM_COMPRESSION)
            m_zlib = azcreate(ZLib, (&AllocatorInstance<OSAllocator>::GetAllocator()), OSAllocator);
            m_zlib->StartDecompressor();
#endif
        }

        //=========================================================================
        // DrillerInputFileStream::DrillerInputFileStream
        // [3/23/2011]
        //=========================================================================
        DrillerInputFileStream::~DrillerInputFileStream()
        {
#if defined(AZ_FILE_STREAM_COMPRESSION)
            azdestroy(m_zlib, OSAllocator);
#endif
        }

        //=========================================================================
        // DrillerInputFileStream::Open
        // [3/23/2011]
        //=========================================================================
        bool DrillerInputFileStream::Open(const char* fileName, int mode, int platformFlags)
        {
            if (IO::SystemFile::Open(fileName, mode, platformFlags))
            {
                DrillerOutputStream::StreamHeader sh;
#if defined(AZ_FILE_STREAM_COMPRESSION)
                // TODO: optional encode the file in the same format as the streamer so they are interchangeable
#endif
                // first read the header of the stream file.
                return ReadHeader();
            }
            return false;
        }
        //=========================================================================
        // DrillerInputFileStream::ReadBinary
        // [3/23/2011]
        //=========================================================================
        unsigned int DrillerInputFileStream::ReadBinary(void* data, unsigned int maxDataSize)
        {
            // make sure the compressed buffer if full enough...
            size_t dataToLoad = maxDataSize * 2;
            m_compressedData.reserve(dataToLoad);
            while (m_compressedData.size() < dataToLoad)
            {
                unsigned char buffer[10 * 1024];
                IO::SystemFile::SizeType bytesRead = Read(AZ_ARRAY_SIZE(buffer), buffer);
                if (bytesRead > 0)
                {
                    m_compressedData.insert(m_compressedData.end(), (unsigned char*)buffer, buffer + bytesRead);
                }
                if (bytesRead < AZ_ARRAY_SIZE(buffer))
                {
                    break;
                }
            }
#if defined(AZ_FILE_STREAM_COMPRESSION)
            unsigned int dataSize = maxDataSize;
            unsigned int bytesProcessed = m_zlib->Decompress(m_compressedData.data(), (unsigned)m_compressedData.size(), data, dataSize);
            unsigned int readSize = maxDataSize - dataSize; // Zlib::Decompress decrements the dataSize parameter by the amount uncompressed
#else
            unsigned int bytesProcessed = AZStd::GetMin((unsigned int)m_compressedData.size(), maxDataSize);
            unsigned int readSize = bytesProcessed;
            memcpy(data, m_compressedData.data(), readSize);
#endif
            m_compressedData.erase(m_compressedData.begin(), m_compressedData.begin() + bytesProcessed);
            return readSize;
        }

        //=========================================================================
        // DrillerInputFileStream::Close
        // [3/23/2011]
        //=========================================================================
        void DrillerInputFileStream::Close()
        {
#if defined(AZ_FILE_STREAM_COMPRESSION)
            if (m_zlib)
            {
                m_zlib->ResetDecompressor();
            }
#endif // AZ_FILE_STREAM_COMPRESSION
            AZ::IO::SystemFile::Close();
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // DrillerSAXParser
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //=========================================================================
        // DrillerSAXParser
        // [3/23/2011]
        //=========================================================================
        DrillerSAXParser::DrillerSAXParser(const TagCallbackType& tcb, const DataCallbackType& dcb)
            : m_tagCallback(tcb)
            , m_dataCallback(dcb)
        {
        }

        //=========================================================================
        // ProcessStream
        // [3/23/2011]
        //=========================================================================
        void
        DrillerSAXParser::ProcessStream(DrillerInputStream& stream)
        {
            static const int processChunkSize = 15 * 1024;
            char buffer[processChunkSize];
            unsigned int dataSize;
            bool isEndianSwap = stream.IsEndianSwap();
            while ((dataSize = stream.ReadBinary(buffer, processChunkSize)) > 0)
            {
                char* dataStart = buffer;
                char* dataEnd = dataStart + dataSize;
                bool dataInBuffer = false;
                if (!m_buffer.empty())
                {
                    m_buffer.insert(m_buffer.end(), dataStart, dataEnd);
                    dataStart = m_buffer.data();
                    dataEnd = dataStart + m_buffer.size();
                    dataInBuffer = true;
                }
                const int entrySize = sizeof(DrillerOutputStream::StreamEntry);
                while (dataStart != dataEnd)
                {
                    if ((dataEnd - dataStart) < entrySize) // we need at least one entry to proceed
                    {
                        // not enough data to process, buffer it.
                        if (!dataInBuffer)
                        {
                            m_buffer.insert(m_buffer.end(), dataStart, dataEnd);
                        }
                        break;
                    }

                    DrillerOutputStream::StreamEntry* se = reinterpret_cast<DrillerOutputStream::StreamEntry*>(dataStart);
                    if (isEndianSwap)
                    {
                        // endian swap
                        AZStd::endian_swap(se->name);
                        AZStd::endian_swap(se->sizeAndFlags);
                    }

                    u32 dataType = (se->sizeAndFlags & DrillerOutputStream::StreamEntry::dataInternalMask) >> DrillerOutputStream::StreamEntry::dataInternalShift;
                    u32 value = se->sizeAndFlags & DrillerOutputStream::StreamEntry::dataSizeMask;
                    Data de;
                    de.m_name = se->name;
                    de.m_stringPool = stream.GetStringPool();
                    de.m_isPooledString = false;
                    de.m_isPooledStringCrc32 = false;
                    switch (dataType)
                    {
                    case DrillerOutputStream::StreamEntry::INT_TAG:
                    {
                        bool isStart = (value != 0);
                        m_tagCallback(se->name, isStart);
                        dataStart += entrySize;
                    } break;
                    case DrillerOutputStream::StreamEntry::INT_DATA_U8:
                    {
                        u8 value8 = static_cast<u8>(value);
                        de.m_data = &value8;
                        de.m_dataSize = 1;
                        de.m_isEndianSwap = false;
                        m_dataCallback(de);
                        dataStart += entrySize;
                    } break;
                    case DrillerOutputStream::StreamEntry::INT_DATA_U16:
                    {
                        u16 value16 = static_cast<u16>(value);
                        de.m_data = &value16;
                        de.m_dataSize = 2;
                        de.m_isEndianSwap = false;
                        m_dataCallback(de);
                        dataStart += entrySize;
                    } break;
                    case DrillerOutputStream::StreamEntry::INT_DATA_U29:
                    {
                        de.m_data = &value;
                        de.m_dataSize = 4;
                        de.m_isEndianSwap = false;
                        m_dataCallback(de);
                        dataStart += entrySize;
                    } break;
                    case DrillerOutputStream::StreamEntry::INT_POOLED_STRING:
                    {
                        unsigned int userDataSize = value;
                        if ((userDataSize + entrySize) <= (unsigned)(dataEnd - dataStart))
                        {
                            // Add string to the pool
                            AZ_Assert(de.m_stringPool != nullptr, "We require a string pool to parse this stream");
                            AZ::u32 crc32;
                            const char* stringPtr;
                            dataStart += entrySize;
                            de.m_stringPool->InsertCopy(reinterpret_cast<const char*>(dataStart), userDataSize, crc32, &stringPtr);
                            de.m_dataSize = userDataSize;
                            de.m_isEndianSwap = isEndianSwap;
                            de.m_isPooledString = true;
                            de.m_data = const_cast<void*>(static_cast<const void*>(stringPtr));
                            m_dataCallback(de);
                            dataStart += userDataSize;
                        }
                        else
                        {
                            // we can't process data right now add it to the buffer (if we have not done that already)
                            if (!dataInBuffer)
                            {
                                m_buffer.insert(m_buffer.end(), dataStart, dataEnd);
                            }
                            dataEnd = dataStart;     // exit the loop
                        }
                    } break;
                    case DrillerOutputStream::StreamEntry::INT_POOLED_STRING_CRC32:
                    {
                        de.m_isPooledStringCrc32 = true;
                        AZ_Assert(value == 4, "The data size for a pooled string crc32 should be 4 bytes!");
                    }     // continue to INT_SIZE
                    case DrillerOutputStream::StreamEntry::INT_SIZE:
                    {
                        unsigned int userDataSize = value;
                        if ((userDataSize + entrySize) <= (unsigned)(dataEnd - dataStart)) // do we have all the date we need to process...
                        {
                            dataStart += entrySize;
                            de.m_data = dataStart;
                            de.m_dataSize = userDataSize;
                            de.m_isEndianSwap = isEndianSwap;
                            m_dataCallback(de);
                            dataStart += userDataSize;
                        }
                        else
                        {
                            // we can't process data right now add it to the buffer (if we have not done that already)
                            if (!dataInBuffer)
                            {
                                m_buffer.insert(m_buffer.end(), dataStart, dataEnd);
                            }
                            dataEnd = dataStart;     // exit the loop
                        }
                    } break;
                    default:
                    {                        
                        AZ_Error("DrillerSAXParser",false,"Encounted unknown symbol (%i) while processing stream (%s). Aborting stream.\n",dataType, stream.GetIdentifier());

                        // If we can't process anything, we want to just escape the loop, to avoid spinning infinitely
                        dataEnd = dataStart;                        
                    } break;
                    }
                }
                if (dataInBuffer) // if the data was in the buffer remove the processed data!
                {
                    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + (dataStart - m_buffer.data()));
                }
            }
        }

        void DrillerSAXParser::Data::Read(AZ::Vector3& v) const
        {
            AZ_Assert(m_dataSize == sizeof(float) * 3, "We are expecting 3 floats for Vector3 element 0x%08x with size %d bytes", m_name, m_dataSize);
            float* data = reinterpret_cast<float*>(m_data);
            if (m_isEndianSwap)
            {
                AZStd::endian_swap(data, data + 3);
                m_isEndianSwap = false;
            }
            v = Vector3::CreateFromFloat3(data);
        }
        void DrillerSAXParser::Data::Read(AZ::Vector4& v) const
        {
            AZ_Assert(m_dataSize == sizeof(float) * 4, "We are expecting 4 floats for Vector4 element 0x%08x with size %d bytes", m_name, m_dataSize);
            float* data = reinterpret_cast<float*>(m_data);
            if (m_isEndianSwap)
            {
                AZStd::endian_swap(data, data + 4);
                m_isEndianSwap = false;
            }
            v = Vector4::CreateFromFloat4(data);
        }
        void DrillerSAXParser::Data::Read(AZ::Aabb& aabb) const
        {
            AZ_Assert(m_dataSize == sizeof(float) * 6, "We are expecting 6 floats for Aabb element 0x%08x with size %d bytes", m_name, m_dataSize);
            float* data = reinterpret_cast<float*>(m_data);
            if (m_isEndianSwap)
            {
                AZStd::endian_swap(data, data + 6);
                m_isEndianSwap = false;
            }
            Vector3 min = Vector3::CreateFromFloat3(data);
            Vector3 max = Vector3::CreateFromFloat3(&data[3]);
            aabb = Aabb::CreateFromMinMax(min, max);
        }
        void DrillerSAXParser::Data::Read(AZ::Obb& obb) const
        {
            AZ_Assert(m_dataSize == sizeof(float) * 10, "We are expecting 10 floats for Obb element 0x%08x with size %d bytes", m_name, m_dataSize);
            float* data = reinterpret_cast<float*>(m_data);
            if (m_isEndianSwap)
            {
                AZStd::endian_swap(data, data + 10);
                m_isEndianSwap = false;
            }
            Vector3 position = Vector3::CreateFromFloat3(data);
            Quaternion rotation = Quaternion::CreateFromFloat4(&data[3]);
            Vector3 halfLengths = Vector3::CreateFromFloat3(&data[7]);
            obb = Obb::CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths);
        }
        void DrillerSAXParser::Data::Read(AZ::Transform& tm) const
        {
            AZ_Assert(m_dataSize == sizeof(float) * 12, "We are expecting 12 floats for Transform element 0x%08x with size %d bytes", m_name, m_dataSize);
            float* data = reinterpret_cast<float*>(m_data);
            if (m_isEndianSwap)
            {
                AZStd::endian_swap(data, data + 12);
                m_isEndianSwap = false;
            }
            const Matrix3x4 matrix3x4 = Matrix3x4::CreateFromRowMajorFloat12(data);
            tm = Transform::CreateFromMatrix3x4(matrix3x4);
        }
        void DrillerSAXParser::Data::Read(AZ::Matrix3x3& tm) const
        {
            AZ_Assert(m_dataSize == sizeof(float) * 9, "We are expecting 9 floats for Matrix3x3 element 0x%08x with size %d bytes", m_name, m_dataSize);
            float* data = reinterpret_cast<float*>(m_data);
            if (m_isEndianSwap)
            {
                AZStd::endian_swap(data, data + 9);
                m_isEndianSwap = false;
            }
            tm = Matrix3x3::CreateFromRowMajorFloat9(data);
        }
        void DrillerSAXParser::Data::Read(AZ::Matrix4x4& tm) const
        {
            AZ_Assert(m_dataSize == sizeof(float) * 16, "We are expecting 16 floats for Matrix4x4 element 0x%08x with size %d bytes", m_name, m_dataSize);
            float* data = reinterpret_cast<float*>(m_data);
            if (m_isEndianSwap)
            {
                AZStd::endian_swap(data, data + 16);
                m_isEndianSwap = false;
            }
            tm = Matrix4x4::CreateFromRowMajorFloat16(data);
        }
        void DrillerSAXParser::Data::Read(AZ::Quaternion& tm) const
        {
            AZ_Assert(m_dataSize == sizeof(float) * 4, "We are expecting 4 floats for Quaternion element 0x%08x with size %d bytes", m_name, m_dataSize);
            float* data = reinterpret_cast<float*>(m_data);
            if (m_isEndianSwap)
            {
                AZStd::endian_swap(data, data + 4);
                m_isEndianSwap = false;
            }
            tm = Quaternion::CreateFromFloat4(data);
        }
        void DrillerSAXParser::Data::Read(AZ::Plane& plane) const
        {
            AZ::Vector4 coeff;
            Read(coeff);
            plane = Plane::CreateFromCoefficients(coeff.GetX(), coeff.GetY(), coeff.GetZ(), coeff.GetW());
        }

        const char*  DrillerSAXParser::Data::PrepareString(unsigned int& stringLength) const
        {
            const char* srcData = reinterpret_cast<const char*>(m_data);
            stringLength = m_dataSize;
            if (m_stringPool)
            {
                AZ::u32 crc32;
                const char* stringPtr;
                if (m_isPooledStringCrc32)
                {
                    crc32 = *reinterpret_cast<AZ::u32*>(m_data);
                    if (m_isEndianSwap)
                    {
                        AZStd::endian_swap(crc32);
                    }
                    stringPtr = m_stringPool->Find(crc32);
                    AZ_Assert(stringPtr != nullptr, "Failed to find string with id 0x%08x in the string pool, proper stream read is impossible!", crc32);
                    stringLength = static_cast<unsigned int>(strlen(stringPtr));
                }
                else if (m_isPooledString)
                {
                    stringPtr = srcData; // already stored in the pool just transfer the pointer
                }
                else
                {
                    // Store copy of the string in the pool to save memory (keep only one reference of the string).
                    m_stringPool->InsertCopy(reinterpret_cast<const char*>(srcData), stringLength, crc32, &stringPtr);
                }
                srcData = stringPtr;
            }
            else
            {
                AZ_Assert(m_isPooledString == false && m_isPooledStringCrc32 == false, "This stream requires using of a string pool as the string is send only once and afterwards only the Crc32 is used!");
            }
            return srcData;
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // DrillerDOMParser
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //=========================================================================
        // Node::GetTag
        // [1/23/2013]
        //=========================================================================
        const DrillerDOMParser::Node* DrillerDOMParser::Node::GetTag(u32 tagName) const
        {
            const Node* tagNode = nullptr;
            for (Node::NodeListType::const_iterator i = m_tags.begin(); i != m_tags.end(); ++i)
            {
                if ((*i).m_name == tagName)
                {
                    tagNode = &*i;
                    break;
                }
            }
            return tagNode;
        }

        //=========================================================================
        // Node::GetData
        // [3/23/2011]
        //=========================================================================
        const DrillerDOMParser::Data* DrillerDOMParser::Node::GetData(u32 dataName) const
        {
            const Data* dataNode = nullptr;
            for (Node::DataListType::const_iterator i = m_data.begin(); i != m_data.end(); ++i)
            {
                if (i->m_name == dataName)
                {
                    dataNode = &*i;
                    break;
                }
            }
            return dataNode;
        }

        //=========================================================================
        // DrillerDOMParser
        // [3/23/2011]
        //=========================================================================
        DrillerDOMParser::DrillerDOMParser(bool isPersistentInputData)
            : DrillerSAXParser(TagCallbackType(this, &DrillerDOMParser::OnTag), DataCallbackType(this, &DrillerDOMParser::OnData))
            , m_isPersistentInputData(isPersistentInputData)
        {
            m_root.m_name = 0;
            m_root.m_parent = nullptr;
            m_topNode = &m_root;
        }
        static int g_numFree = 0;
        //=========================================================================
        // ~DrillerDOMParser
        // [3/23/2011]
        //=========================================================================
        DrillerDOMParser::~DrillerDOMParser()
        {
            DeleteNode(m_root);
        }

        //=========================================================================
        // OnTag
        // [3/23/2011]
        //=========================================================================
        void
        DrillerDOMParser::OnTag(AZ::u32 name, bool isOpen)
        {
            if (isOpen)
            {
                m_topNode->m_tags.push_back();
                Node& node = m_topNode->m_tags.back();
                node.m_name = name;
                node.m_parent = m_topNode;

                m_topNode = &node;
            }
            else
            {
                AZ_Assert(m_topNode->m_name == name, "We have opened tag with name 0x%08x and closing with name 0x%08x", m_topNode->m_name, name);
                m_topNode = m_topNode->m_parent;
            }
        }
        //=========================================================================
        // OnData
        // [3/23/2011]
        //=========================================================================
        void
        DrillerDOMParser::OnData(const Data& data)
        {
            Data de = data;
            if (!m_isPersistentInputData)
            {
                de.m_data = azmalloc(data.m_dataSize, 1, OSAllocator);
                memcpy(const_cast<void*>(de.m_data), data.m_data, data.m_dataSize);
            }
            m_topNode->m_data.push_back(de);
        }
        //=========================================================================
        // DeleteNode
        // [3/23/2011]
        //=========================================================================
        void
        DrillerDOMParser::DeleteNode(Node& node)
        {
            if (!m_isPersistentInputData)
            {
                for (Node::DataListType::iterator iter = node.m_data.begin(); iter != node.m_data.end(); ++iter)
                {
                    azfree(iter->m_data, OSAllocator, iter->m_dataSize);
                    ++g_numFree;
                }
                node.m_data.clear();
            }
            for (Node::NodeListType::iterator iter = node.m_tags.begin(); iter != node.m_tags.end(); ++iter)
            {
                DeleteNode(*iter);
            }
            node.m_tags.clear();
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // DrillerSAXParserHandler
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //=========================================================================
        // DrillerSAXParserHandler
        // [3/14/2013]
        //=========================================================================
        DrillerSAXParserHandler::DrillerSAXParserHandler(DrillerHandlerParser* rootHandler)
            : DrillerSAXParser(TagCallbackType(this, &DrillerSAXParserHandler::OnTag), DataCallbackType(this, &DrillerSAXParserHandler::OnData))
        {
            // Push the root element
            m_stack.push_back(rootHandler);
        }

        //=========================================================================
        // OnTag
        // [3/14/2013]
        //=========================================================================
        void DrillerSAXParserHandler::OnTag(u32 name, bool isOpen)
        {
            if (m_stack.size() == 0)
            {
                return;
            }

            DrillerHandlerParser* childHandler = nullptr;
            DrillerHandlerParser* currentHandler = m_stack.back();
            if (isOpen)
            {
                if (currentHandler != nullptr)
                {
                    childHandler = currentHandler->OnEnterTag(name);
                    AZ_Warning("Driller", !currentHandler->IsWarnOnUnsupportedTags() || childHandler != nullptr, "Could not find handler for tag 0x%08x", name);
                }
                m_stack.push_back(childHandler);
            }
            else
            {
                m_stack.pop_back();
                if (m_stack.size() > 0)
                {
                    DrillerHandlerParser* parentHandler = m_stack.back();
                    if (parentHandler)
                    {
                        parentHandler->OnExitTag(currentHandler, name);
                    }
                }
            }
        }

        //=========================================================================
        // OnData
        // [3/14/2013]
        //=========================================================================
        void DrillerSAXParserHandler::OnData(const DrillerSAXParser::Data& data)
        {
            if (m_stack.size() == 0)
            {
                return;
            }

            DrillerHandlerParser* currentHandler = m_stack.back();
            if (currentHandler)
            {
                currentHandler->OnData(data);
            }
        }
    } // namespace Debug
} // namespace AZ
