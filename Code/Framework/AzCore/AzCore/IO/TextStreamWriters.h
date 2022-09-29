/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::IO
{
    /**
     * For use with rapidxml::print.
     */
    class RapidXMLStreamWriter final
    {
    public:
        class PrintIterator final
        {
        public:
            explicit PrintIterator(RapidXMLStreamWriter* writer)
                : m_writer(writer)
            {
            }

            PrintIterator(const PrintIterator& it) = default;

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

        RapidXMLStreamWriter(IO::GenericStream* stream, size_t defaultWriteCacheSize = 128 * 1024)
            : m_stream(stream)
        {
            m_cache.reserve(defaultWriteCacheSize);
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
                    AZ_Error("Serializer", false, "Failed writing %zu byte(s) to stream, wrote only %llu bytes!", m_cache.size(), bytes);
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
        int m_errorCount{};
    };

    //! Implements the rapidjson::Stream concept
    class RapidJSONWriteStreamUnbuffered
    {
    public:
        using Ch = char;    //!< Character type. Only support char.

        explicit RapidJSONWriteStreamUnbuffered(AZ::IO::GenericStream& stream)
            : m_stream(&stream)
        {
        }

        virtual ~RapidJSONWriteStreamUnbuffered() = default;

        RapidJSONWriteStreamUnbuffered(const RapidJSONWriteStreamUnbuffered&) = delete;
        RapidJSONWriteStreamUnbuffered& operator=(const RapidJSONWriteStreamUnbuffered&) = delete;

        virtual void Put(char c)
        {
            if (IO::SizeType bytes = m_stream->Write(1, &c); bytes != 1)
            {
                AZ_Error("Serializer", false, "Failed to add byte to stream, stream current size is %zu", m_stream->GetLength());
            }
        }

        virtual void Flush()
        {
            // GenericStream doesn't support a flush API
        }

        // Not implemented
        char Peek() const
        {
            AZ_Assert(false, "RapidJSONStreamWriter Peek not supported.");
            return {};
        }

        char Take()
        {
            AZ_Assert(false, "RapidJSONStreamWriter Take not supported.");
            return {};
        }
        size_t Tell() const
        {
            AZ_Assert(false, "RapidJSONStreamWriter Tell not supported.");
            return 0;
        }
        char* PutBegin()
        {
            AZ_Assert(false, "RapidJSONStreamWriter PutBegin not supported.");
            return nullptr;
        }
        size_t PutEnd(char*)
        {
            AZ_Assert(false, "RapidJSONStreamWriter PutEnd not supported.");
            return 0;
        }

        AZ::IO::GenericStream* m_stream;
    };

    //! Adds caching around the RapidJsonStreamWriter
    class RapidJSONStreamWriter
        : public RapidJSONWriteStreamUnbuffered
    {
    public:
        explicit RapidJSONStreamWriter(AZ::IO::GenericStream* stream, size_t writeCacheSize = 128 * 1024)
            : RapidJSONWriteStreamUnbuffered(*stream)
        {
            m_cache.reserve(writeCacheSize);
        }

        ~RapidJSONStreamWriter() override
        {
            FlushCache();
        }

        void Put(char c) override
        {
            if (m_cache.size() == m_cache.capacity())
            {
                FlushCache();
            }
            m_cache.push_back(c);
        }

        void Flush() override
        {
            FlushCache();
        }

    private:
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
        AZStd::vector<AZ::u8> m_cache;
    };
}   // namespace AZ::IO
