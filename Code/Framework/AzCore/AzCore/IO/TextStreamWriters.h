/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZCORE_IO_TEXTSTREAMWRITERS_H
#define AZCORE_IO_TEXTSTREAMWRITERS_H

#include <AzCore/base.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace IO
    {
        /**
         * For use with rapidxml::print.
         */
        class RapidXMLStreamWriter
        {
        public:
            class PrintIterator
            {
            public:
                PrintIterator(RapidXMLStreamWriter* writer)
                    : m_writer(writer)
                {
                }

                PrintIterator(const PrintIterator& it)
                    : m_writer(it.m_writer)
                {
                }

                PrintIterator& operator=(const PrintIterator& rhs) = default;

                PrintIterator& operator*() { return *this; }
                PrintIterator& operator++() { return *this; }
                PrintIterator& operator++(int) { return *this; }
                PrintIterator& operator=(char c)
                {
                    if (m_writer->m_cache.size() == m_writer->m_cache.capacity())
                    {
                        m_writer->FlushCache();
                    }
                    m_writer->m_cache.push_back(c);
                    return *this;
                }
            private:
                RapidXMLStreamWriter* m_writer;
            };

            RapidXMLStreamWriter(IO::GenericStream* stream, size_t writeCacheSize = 128 * 1024)
                : m_stream(stream)
                , m_errorCount(0) 
            {
                m_cache.reserve(writeCacheSize);
            }

            ~RapidXMLStreamWriter()
            {
                FlushCache();
            }

            RapidXMLStreamWriter(const RapidXMLStreamWriter&) = delete;
            RapidXMLStreamWriter& operator=(const RapidXMLStreamWriter&) = delete;

            void FlushCache()
            {
                if (!m_cache.empty())
                {
                    IO::SizeType bytes = m_stream->Write(m_cache.size(), m_cache.data());
                    if (bytes != m_cache.size())
                    {
                        AZ_Error("Serializer", false, "Failed writing %d byte(s) to stream, wrote only %d bytes!", m_cache.size(), bytes);
                        ++m_errorCount;
                    }
                    m_cache.clear();
                }
            }

            PrintIterator Iterator()
            {
                return PrintIterator(this);
            }

            IO::GenericStream* m_stream;
            AZStd::vector<AZ::u8> m_cache;
            int m_errorCount;
        };

        /**
         * For use with rapidxml::Writer/PrettyWriter.
         */
        class RapidJSONStreamWriter
        {
        public:
            typedef char Ch;    //!< Character type. Only support char.

            RapidJSONStreamWriter(AZ::IO::GenericStream* stream, size_t writeCacheSize = 128 * 1024)
                : m_stream(stream)
            {
                m_cache.reserve(writeCacheSize);
            }

            ~RapidJSONStreamWriter()
            {
                FlushCache();
            }

            RapidJSONStreamWriter(const RapidJSONStreamWriter&) = delete;
            RapidJSONStreamWriter& operator=(const RapidJSONStreamWriter&) = delete;

            void FlushCache()
            {
                if (!m_cache.empty())
                {
                    IO::SizeType bytes = m_stream->Write(m_cache.size(), m_cache.data());
                    if (bytes != m_cache.size())
                    {
                        AZ_Error("Serializer", false, "Failed writing %d byte(s) to stream, wrote only %d bytes!", m_cache.size(), bytes);
                    }
                    m_cache.clear();
                }
            }

            void Put(char c)
            {
                if (m_cache.size() == m_cache.capacity())
                {
                    FlushCache();
                }
                m_cache.push_back(c);
            }

            void PutN(char c, size_t n)
            {
                while (n > 0)
                {
                    Put(c); // if this is common we can optimize the write to the cache
                    --n;
                }
            }

            void Flush()
            {
                FlushCache();
            }

            // Not implemented
            char Peek() const
            {
                AZ_Assert(false, "RapidJSONStreamWriter Peek not supported.");
                return 0;
            }

            char Take()
            {
                AZ_Assert(false, "RapidJSONStreamWriter Take not supported.");
                return 0;
            }
            size_t Tell() const
            {
                AZ_Assert(false, "RapidJSONStreamWriter Tell not supported.");
                return 0;
            }
            char* PutBegin()
            {
                AZ_Assert(false, "RapidJSONStreamWriter PutBegin not supported.");
                return 0;
            }
            size_t PutEnd(char*)
            {
                AZ_Assert(false, "RapidJSONStreamWriter PutEnd not supported.");
                return 0;
            }

            AZ::IO::GenericStream* m_stream;
            AZStd::vector<AZ::u8> m_cache;
        };
    }   // namespace IO
}   // namespace AZ

#endif  // AZCORE_IO_TEXTSTREAMWRITERS_H
#pragma once
