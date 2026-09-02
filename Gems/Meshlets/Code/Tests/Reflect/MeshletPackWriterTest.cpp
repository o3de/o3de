/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>
#include <AzCore/std/containers/vector.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>
#include <Meshlets/Reflect/MeshletPackWriter.h>
#include <cstring>

namespace UnitTest
{
    using namespace AZ::Meshlets;

    TEST(MeshletPackWriter, EmitsHeaderWithCorrectMagicAndVersion)
    {
        MeshletPackWriter writer;
        writer.BeginPack();
        // No sections added.
        AZStd::vector<AZ::u8> bytes;
        ASSERT_TRUE(writer.End(bytes));

        ASSERT_GE(bytes.size(), sizeof(FileHeader));
        FileHeader h;
        std::memcpy(&h, bytes.data(), sizeof(h));
        EXPECT_EQ(PackMagic, h.m_magic);
        EXPECT_EQ(PackVersion, h.m_version);
        EXPECT_EQ(0u, h.m_tocCount);
        EXPECT_EQ(0u, h.m_flags);
    }

    TEST(MeshletPackWriter, SectionOffsetsAre16ByteAligned)
    {
        MeshletPackWriter writer;
        writer.BeginPack();

        // Three sections of varying sizes that exercise alignment padding.
        AZ::u8 a[7]  = { 1, 2, 3, 4, 5, 6, 7 };
        AZ::u8 b[33] = {};
        AZ::u8 c[1]  = { 9 };
        writer.AddSection(SectionKind::PackHeader, a, sizeof(a));
        writer.AddSection(SectionKind::MeshDescriptors, b, sizeof(b));
        writer.AddSection(SectionKind::ClusterDescriptors, c, sizeof(c));

        AZStd::vector<AZ::u8> bytes;
        ASSERT_TRUE(writer.End(bytes));

        const AZ::u8* p = bytes.data();
        FileHeader h;
        std::memcpy(&h, p, sizeof(h));
        ASSERT_EQ(3u, h.m_tocCount);

        const SectionTocEntry* toc = reinterpret_cast<const SectionTocEntry*>(p + sizeof(FileHeader));
        for (AZ::u32 i = 0; i < h.m_tocCount; ++i)
        {
            EXPECT_EQ(0u, toc[i].m_offset % SectionAlignment)
                << "section " << i << " offset " << toc[i].m_offset << " not 16-byte aligned";
            EXPECT_LE(toc[i].m_offset + toc[i].m_size, bytes.size());
        }
    }

    TEST(MeshletPackWriter, SectionDataReadsBackByteIdentical)
    {
        MeshletPackWriter writer;
        writer.BeginPack();
        AZ::u8 payload[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE };
        writer.AddSection(SectionKind::TriangleIndices, payload, sizeof(payload));

        AZStd::vector<AZ::u8> bytes;
        ASSERT_TRUE(writer.End(bytes));

        const AZ::u8* p = bytes.data();
        const SectionTocEntry* toc = reinterpret_cast<const SectionTocEntry*>(p + sizeof(FileHeader));
        ASSERT_EQ(static_cast<AZ::u32>(SectionKind::TriangleIndices), toc[0].m_kind);
        EXPECT_EQ(0, std::memcmp(p + toc[0].m_offset, payload, sizeof(payload)));
    }
}
