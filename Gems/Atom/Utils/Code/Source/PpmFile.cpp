/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/PpmFile.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/std/limits.h>

namespace AZ
{
    AZStd::vector<uint8_t> Utils::PpmFile::CreatePpmFromImageBuffer(AZStd::span<const uint8_t> buffer, const RHI::Size& size, RHI::Format format)
    {
        AZ_Assert(format == RHI::Format::R8G8B8A8_UNORM || format == RHI::Format::B8G8R8A8_UNORM, "CreatePpmFromImageReadbackResult only supports R8G8B8A8_UNORM");

        const uint32_t pixelHeight = size.m_height;
        const uint32_t pixelWidth = size.m_width;

        constexpr size_t rbgByteSize = 3;
        constexpr size_t rbgaByteSize = 4;

        const uint64_t pixelCount64 = static_cast<uint64_t>(pixelWidth) * static_cast<uint64_t>(pixelHeight);
        if (pixelCount64 > AZStd::numeric_limits<size_t>::max() / rbgaByteSize)
        {
            AZ_Error("PpmFile", false, "Image dimensions overflow size_t (%u x %u)", pixelWidth, pixelHeight);
            return {};
        }

        const size_t pixelCount = static_cast<size_t>(pixelCount64);
        const size_t requiredInputBytes = pixelCount * rbgaByteSize;
        if (buffer.size() < requiredInputBytes)
        {
            AZ_Error("PpmFile", false, "Input image buffer size (%zu bytes) is smaller than required (%zu bytes)", buffer.size(), requiredInputBytes);
            return {};
        }

        // Header for binary .ppm format, RGB, 8 bit per color component
        const auto header = AZStd::string::format("P6\n%u %u\n255\n", pixelWidth, pixelHeight);
        const size_t payloadBytes = pixelCount * rbgByteSize;
        if (header.size() > AZStd::numeric_limits<size_t>::max() - payloadBytes)
        {
            AZ_Error("PpmFile", false, "Output PPM size overflows size_t");
            return {};
        }

        AZStd::vector<uint8_t> outBuffer; // write this buffer out once at the end
        outBuffer.resize(header.size() + payloadBytes);

        ::memcpy(outBuffer.data(), header.data(), header.size());

        // throwing away alpha channel, since .ppm doesn't support it
        const uint8_t* inData = buffer.data();
        uint8_t* outData = outBuffer.data() + header.size();
        for (size_t i = 0; i < pixelCount; ++i)
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

        auto readInt = [&]() -> uint32_t
        {
            eatWhitepaceAndComments();

            uint64_t value = 0;
            bool hasDigits = false;
            while (pos < ppmData.size())
            {
                const uint8_t byte = ppmData[pos];
                if (byte < '0' || byte > '9')
                {
                    break;
                }
                hasDigits = true;
                value = value * 10 + (byte - '0');
                if (value > AZStd::numeric_limits<uint32_t>::max())
                {
                    return 0;
                }
                ++pos;
            }

            return hasDigits ? static_cast<uint32_t>(value) : 0;
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

        if (pos >= ppmData.size() || !isWhitespace(static_cast<char>(ppmData[pos])))
        {
            AZ_Error("PpmFile", false, "Invalid PPM header terminator");
            return false;
        }
        ++pos; // Consume final header character, usually \n

        constexpr size_t rbgByteSize = 3;
        constexpr size_t rbgaByteSize = 4;

        const uint64_t pixelCount64 = static_cast<uint64_t>(size.m_width) * static_cast<uint64_t>(size.m_height);
        if (pixelCount64 > AZStd::numeric_limits<size_t>::max() / rbgaByteSize)
        {
            AZ_Error("PpmFile", false, "Image dimensions overflow size_t (%u x %u)", size.m_width, size.m_height);
            return false;
        }

        const size_t pixelCount = static_cast<size_t>(pixelCount64);
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

        for (size_t i = 0; i < pixelCount; ++i)
        {
            ::memcpy(outData, inData, rbgByteSize);

            outData += rbgaByteSize;
            inData += rbgByteSize;
        }

        return true;
    }
}
