/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/PpmFile.h>
#include <AzCore/IO/ByteContainerStream.h>

namespace AZ
{
    AZStd::vector<uint8_t> Utils::PpmFile::CreatePpmFromImageBuffer(AZStd::span<const uint8_t> buffer, const RHI::Size& size, RHI::Format format)
    {
        AZ_Assert(format == RHI::Format::R8G8B8A8_UNORM || format == RHI::Format::B8G8R8A8_UNORM, "CreatePpmFromImageReadbackResult only supports R8G8B8A8_UNORM");

        const uint32_t pixelHeight = size.m_height;
        const uint32_t pixelWidth = size.m_width;

        constexpr int rbgByteSize = 3;
        constexpr int rbgaByteSize = 4;

        // Header for binary .ppm format, RGB, 8 bit per color component
        const auto header = AZStd::string::format("P6\n%d %d\n255\n", pixelWidth, pixelHeight);

        AZStd::vector<uint8_t> outBuffer; // write this buffer out once at the end
        outBuffer.resize(header.size() + pixelWidth * pixelHeight * rbgByteSize);

        ::memcpy(outBuffer.data(), header.data(), header.size());

        // throwing away alpha channel, since .ppm doesn't support it
        const uint8_t* inData = buffer.data();
        uint8_t* outData = outBuffer.data() + header.size();
        for (uint32_t i = 0; i < pixelHeight * pixelWidth; ++i)
        {
            ::memcpy(outData, inData, rbgByteSize);
            if (format == RHI::Format::B8G8R8A8_UNORM)
            {
                AZStd::reverse(outData, outData + rbgByteSize);
            }
            outData += rbgByteSize;
            inData += rbgaByteSize;
        }

        return outBuffer;
    }

    bool Utils::PpmFile::CreateImageBufferFromPpm(AZStd::span<const uint8_t> ppmData, AZStd::vector<uint8_t>& buffer, RHI::Size& size, RHI::Format& format)
    {
        if (ppmData.size() < 2)
        {
            return false;
        }

        if (ppmData[0] != 'P' || ppmData[1] != '6')
        {
            return false;
        }

        size_t pos = 2;

        auto isWhitespace = [](char c)
        {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        };

        auto eatWhitepaceAndComments = [&]()
        {
            bool isComment = false;

            while (pos < ppmData.size())
            {
                uint8_t byte = ppmData[pos];

                if (byte == '#')
                {
                    isComment = true;
                }
                else if (byte == '\n')
                {
                    isComment = false;
                }

                if (isWhitespace(byte) || isComment)
                {
                    ++pos;
                }
                else
                {
                    break;
                }
            }
        };

        auto readInt = [&]()
        {
            eatWhitepaceAndComments();

            const char* startOfInt = reinterpret_cast<const char*>(&ppmData[pos]);
            char* endOfInt = nullptr;
            uint32_t value = static_cast<uint32_t>(strtoul(startOfInt, &endOfInt, 10));
            pos += endOfInt - startOfInt;

            return value;
        };


        size.m_width = readInt();
        size.m_height = readInt();
        uint32_t maxValue = readInt();

        if (size.m_width == 0)
        {
            AZ_Error("PpmFile", false, "Invalid dimensions");
            return false;
        }

        if (size.m_height == 0)
        {
            AZ_Error("PpmFile", false, "Invalid dimensions");
            return false;
        }

        if (maxValue != 255)
        {
            AZ_Error("PpmFile", false, "Invalid max channel value");
            return false;
        }

        ++pos; // Consume final header character, usually \n

        constexpr int rbgByteSize = 3;
        constexpr int rbgaByteSize = 4;

        const size_t pixelCount = size.m_width * size.m_height;
        const size_t expectedBytes = pixelCount * rbgByteSize;
        const size_t remainingBytes = ppmData.size() - pos;
        if (remainingBytes != expectedBytes)
        {
            AZ_Error("PpmFile", false, "Image payload size (%zu bytes) does not match size indicated in the header (%zu bytes, %u x %u)",
                remainingBytes, expectedBytes, size.m_width, size.m_height);
            return false;
        }

        buffer.resize(pixelCount * rbgaByteSize);
        format = RHI::Format::R8G8B8A8_UNORM;

        const uint8_t* inData = ppmData.data() + pos;
        uint8_t* outData = buffer.data();

        for (uint32_t i = 0; i < pixelCount; ++i)
        {
            ::memcpy(outData, inData, rbgByteSize);

            outData += rbgaByteSize;
            inData += rbgByteSize;
        }

        return true;
    }
}
