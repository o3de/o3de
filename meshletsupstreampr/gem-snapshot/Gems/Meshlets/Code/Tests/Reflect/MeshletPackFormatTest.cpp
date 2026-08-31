/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>
#include <cstring>

namespace UnitTest
{
    using namespace AZ::Meshlets;

    TEST(MeshletPackFormat, MagicConstantIsMTLPLittleEndian)
    {
        // Magic must read as 'M','T','L','P' in file order (little-endian u32).
        const char* bytes = reinterpret_cast<const char*>(&PackMagic);
        EXPECT_EQ('M', bytes[0]);
        EXPECT_EQ('T', bytes[1]);
        EXPECT_EQ('L', bytes[2]);
        EXPECT_EQ('P', bytes[3]);
    }

    TEST(MeshletPackFormat, FileHeaderRoundTripsThroughMemcpy)
    {
        FileHeader h;
        h.m_magic = PackMagic;
        h.m_version = PackVersion;
        h.m_tocCount = 6;
        h.m_flags = 0;
        h.m_reserved = 0;

        // Round-trip through a byte buffer
        AZ::u8 buf[sizeof(FileHeader)];
        std::memcpy(buf, &h, sizeof(h));
        FileHeader decoded;
        std::memcpy(&decoded, buf, sizeof(decoded));

        EXPECT_EQ(PackMagic, decoded.m_magic);
        EXPECT_EQ(PackVersion, decoded.m_version);
        EXPECT_EQ(6u, decoded.m_tocCount);
        EXPECT_EQ(0u, decoded.m_flags);
    }

    TEST(MeshletPackFormat, SectionAlignmentIs16)
    {
        EXPECT_EQ(16u, SectionAlignment);
    }
}
