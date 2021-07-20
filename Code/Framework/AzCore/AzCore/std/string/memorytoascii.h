/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_MEMORYTOASCII_H
#define AZSTD_MEMORYTOASCII_H

#include <AzCore/std/base.h>
#include <AzCore/std/string/string.h>

namespace AZStd
{
    namespace MemoryToASCII
    {
        /*
        ** Handy method to make a memory like display of a buffer for console output. Usually 4 or 5 lines of binary and ascii data. Data width is a default
        ** of 16 bytes, can be any value but a min width of 3 to any value. Offsets are always in hex and 6 digits are displayed. The maxShowSize will limit
        ** output to a predetermined size.
        ** 
        ** Output can be formatted using the predefined Options
        ** Example output:
        **
        ** Address: 0x00000296A8D300B0 Data Size:79 Max Size:256                    <----- Options::Info
        ** Offset                      Data                            ASCII        <--+-- Options::Header 
        ** ------ 00-01-02-03-04-05-06-07-08-09-0a-0b-0c-0d-0e-0f ----------------  <--+   (2 lines)
        ** 000000 29 23 be 84 e1 6c d6 ae 52 90 49 f1 f1 bb e9 eb )#   l  R I
        ** 000010 b3 a6 db 3c 87 0c 3e 99 24 5e 0d 1c 06 b7 47 de    <  > $^    G
        ** 000020 b3 12 4d c8 43 bb 8b a6 1f 03 5a 7d 09 38 25 1f   M C     Z} 8%
        ** 000030 5d d4 cb fc 96 f5 45 3b 13 0d 89 0a 1c db ae 32 ]     E;       2
        ** 000040 20 9a 50 ee 40 78 36 fd 12 49 32 f6 9e 7d 49 dc   P @x6  I2  }I
        ** --+--- ----------------------+------------------------ -------+--------
        **   |                          |                                |
        **   +---- Options::Offset      +---- Options::Binary            +---- Options::ASCII
        */

        enum class Options {
            Header = 0x01,
            Offset = 0x02,
            Binary = 0x04,
            ASCII = 0x08,
            Info = 0x16,
            OffsetHeaderBinaryASCII = Header | Offset | Binary | ASCII,
            OffsetBinaryASCII = Offset | Binary | ASCII,
            OffsetASCII = Offset | ASCII,
            OffsetBinary = Offset | Binary,
            HeaderASCII = Header | ASCII,
            HeaderBinary = Header | Binary,
            OffsetHeaderInfoBinaryASCII = Header | Info | Offset | Binary | ASCII,
            HeaderInfoASCII = Header | Info | ASCII,
            HeaderInfoBinary = Header | Info | Binary,
            Default = OffsetHeaderBinaryASCII
        };

        AZStd::string ToString(const void* memoryAddrs, AZStd::size_t dataSize, AZStd::size_t maxShowSize, AZStd::size_t dataWidth = 16, Options format = Options::Default);
    }
}

#endif // AZSTD_MEMORYTOASCII_H
