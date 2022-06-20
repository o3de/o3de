/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/memorytoascii.h>

namespace AZStd::MemoryToASCII
{
    AZStd::string ToString(const void* memoryAddrs, AZStd::size_t dataSize, AZStd::size_t maxShowSize, AZStd::size_t dataWidth/*=16*/, Options format/*=Options::Default*/)
    {
        AZStd::string output;

        if ((memoryAddrs != nullptr) && (dataSize > 0))
        {
            const AZ::u8 *data = reinterpret_cast<const AZ::u8*>(memoryAddrs);

            if (static_cast<unsigned>(format) != 0)
            {
                output.reserve(8162);

                bool showHeader = static_cast<unsigned>(format) & static_cast<unsigned>(Options::Header) ? true : false;
                bool showOffset = static_cast<unsigned>(format) & static_cast<unsigned>(Options::Offset) ? true : false;
                bool showBinary = static_cast<unsigned>(format) & static_cast<unsigned>(Options::Binary) ? true : false;
                bool showASCII = static_cast<unsigned>(format) & static_cast<unsigned>(Options::ASCII) ? true : false;
                bool showInfo = static_cast<unsigned>(format) & static_cast<unsigned>(Options::Info) ? true : false;

                // Because of the auto formatting for the headers, the min width is 3
                if (dataWidth < 3)
                {
                    dataWidth = 3;
                }

                if (showHeader)
                {
                    AZStd::string line1;
                    AZStd::string line2;
                    line1.reserve(1024);
                    line2.reserve(1024);

                    if (showOffset)
                    {
                        line1 += "Offset";
                        line2 += "------";

                        if (showBinary || showASCII)
                        {
                            line1 += " ";
                            line2 += " ";
                        }
                    }

                    if (showBinary)
                    {
                        static const char *kHeaderName = "Data";
                        static AZStd::size_t kHeaderNameSize = 4;

                        AZStd::size_t lineLength = (dataWidth * 3) - 1;
                        AZStd::size_t numPreSpaces = (lineLength - kHeaderNameSize) / 2;
                        AZStd::size_t numPostSpaces = lineLength - numPreSpaces - kHeaderNameSize;

                        line1 += AZStd::string(numPreSpaces, ' ') + kHeaderName + AZStd::string(numPostSpaces, ' ');
                        //line2 += AZStd::string(lineLength, '-');
                        for(size_t i=0; i<dataWidth; i++)
                        {
                            if (i > 0)
                            {
                                line2 += "-";
                            }

                            line2 += AZStd::string::format("%02zx", i);
                        }

                        if (showASCII)
                        {
                            line1 += " ";
                            line2 += " ";
                        }
                    }

                    if (showASCII)
                    {
                        static const char *kHeaderName = "ASCII";
                        static AZStd::size_t kHeaderNameSize = 5;

                        AZStd::size_t numPreSpaces = (dataWidth - kHeaderNameSize) / 2;
                        AZStd::size_t numPostSpaces = dataWidth - numPreSpaces - kHeaderNameSize;

                        line1 += AZStd::string(numPreSpaces, ' ') + kHeaderName + AZStd::string(numPostSpaces, ' ');
                        line2 += AZStd::string(dataWidth, '-');
                    }

                    if (showInfo)
                    {
                        output += AZStd::string::format("Address: 0x%p Data Size:%zu Max Size:%zu\n", data, dataSize, maxShowSize);
                    }

                    output += line1 + "\n";
                    output += line2 + "\n";
                }

                AZStd::size_t offset = 0;
                AZStd::size_t maxSize = dataSize > maxShowSize ? maxShowSize : dataSize;

                while (offset < maxSize)
                {
                    if (showOffset)
                    {
                        output += AZStd::string::format("%06zx", offset);

                        if (showBinary || showASCII)
                        {
                            output += " ";
                        }
                    }

                    if (showBinary)
                    {
                        AZStd::string binLine;
                        binLine.reserve((dataWidth * 3) * 2);

                        for (AZStd::size_t index = 0; index < dataWidth; index++)
                        {
                            if (!binLine.empty())
                            {
                                binLine += " ";
                            }

                            if ((offset + index) < maxSize)
                            {
                                binLine += AZStd::string::format("%02x", data[offset + index]);
                            }
                            else
                            {
                                binLine += "  ";
                            }
                        }

                        output += binLine;

                        if (showASCII)
                        {
                            output += " ";
                        }
                    }

                    if (showASCII)
                    {
                        AZStd::string asciiLine;
                        asciiLine.reserve(dataWidth * 2);

                        for (AZStd::size_t index = 0; index < dataWidth; index++)
                        {
                            if ((offset + index) > maxSize)
                            {
                                break;
                            }
                            else
                            {
                                char value = static_cast<char>(data[offset + index]);

                                if ((value < 32) || (value > 127))
                                    value = ' ';

                                asciiLine += value;
                            }
                        }

                        output += asciiLine;
                    }

                    output += "\n";
                    offset += dataWidth;
                }
            }
        }

        return output;
    }
} // namespace AZStd::MemoryToASCII
