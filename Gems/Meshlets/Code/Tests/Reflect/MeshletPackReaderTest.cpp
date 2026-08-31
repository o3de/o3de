/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>
#include <AzCore/std/containers/vector.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>
#include <Meshlets/Reflect/MeshletPackReader.h>
#include <Meshlets/Reflect/MeshletPackWriter.h>

namespace UnitTest
{
    using namespace AZ::Meshlets;

    TEST(MeshletPackReader, ParsesValidWrittenPack)
    {
        MeshletPackWriter w;
        w.BeginPack();
        AZ::u8 a[10] = {};
        AZ::u8 b[3] = { 0xAA, 0xBB, 0xCC };
        w.AddSection(SectionKind::PackHeader, a, sizeof(a));
        w.AddSection(SectionKind::TriangleIndices, b, sizeof(b));
        AZStd::vector<AZ::u8> bytes;
        ASSERT_TRUE(w.End(bytes));

        MeshletPackReader r;
        ASSERT_TRUE(r.Parse(bytes.data(), bytes.size()));
        EXPECT_EQ(2u, r.GetSectionCount());
        EXPECT_TRUE(r.HasSection(SectionKind::PackHeader));
        EXPECT_TRUE(r.HasSection(SectionKind::TriangleIndices));
        EXPECT_FALSE(r.HasSection(SectionKind::ClusterDescriptors));

        AZStd::span<const AZ::u8> tri = r.GetSection(SectionKind::TriangleIndices);
        ASSERT_EQ(3u, tri.size());
        EXPECT_EQ(0xAA, tri[0]);
        EXPECT_EQ(0xBB, tri[1]);
        EXPECT_EQ(0xCC, tri[2]);
    }

    TEST(MeshletPackReader, RejectsBadMagic)
    {
        AZStd::vector<AZ::u8> bytes(sizeof(FileHeader), 0);
        FileHeader bad{};
        bad.m_magic = 0xDEADBEEF;
        bad.m_version = PackVersion;
        std::memcpy(bytes.data(), &bad, sizeof(bad));

        MeshletPackReader r;
        EXPECT_FALSE(r.Parse(bytes.data(), bytes.size()));
    }

    TEST(MeshletPackReader, RejectsUnsupportedVersion)
    {
        MeshletPackWriter w;
        w.BeginPack();
        AZStd::vector<AZ::u8> bytes;
        ASSERT_TRUE(w.End(bytes));

        // Tamper with version.
        FileHeader h;
        std::memcpy(&h, bytes.data(), sizeof(h));
        h.m_version = 999;
        std::memcpy(bytes.data(), &h, sizeof(h));

        MeshletPackReader r;
        EXPECT_FALSE(r.Parse(bytes.data(), bytes.size()));
    }

    TEST(MeshletPackReader, RejectsTruncatedFile)
    {
        MeshletPackWriter w;
        w.BeginPack();
        AZ::u8 payload[100] = {};
        w.AddSection(SectionKind::TriangleIndices, payload, sizeof(payload));
        AZStd::vector<AZ::u8> bytes;
        ASSERT_TRUE(w.End(bytes));

        // Truncate the buffer mid-section.
        bytes.resize(bytes.size() - 50);

        MeshletPackReader r;
        EXPECT_FALSE(r.Parse(bytes.data(), bytes.size()));
    }

    TEST(MeshletPackReader, IgnoresUnknownSectionKindsForwardCompat)
    {
        // Future SP2-SP6 packs may include kinds 6-12. A v1 reader must accept
        // them silently (forward-compat per spec §5.5 / north-star §5.5).
        MeshletPackWriter w;
        w.BeginPack();
        AZ::u8 known[5] = {};
        AZ::u8 future[7] = { 1,2,3,4,5,6,7 };
        w.AddSection(SectionKind::PackHeader, known, sizeof(known));
        w.AddSection(SectionKind::ConeBounds /*kind 6, reserved for SP2*/, future, sizeof(future));
        AZStd::vector<AZ::u8> bytes;
        ASSERT_TRUE(w.End(bytes));

        MeshletPackReader r;
        EXPECT_TRUE(r.Parse(bytes.data(), bytes.size()));
        EXPECT_TRUE(r.HasSection(SectionKind::PackHeader));
        EXPECT_TRUE(r.HasSection(SectionKind::ConeBounds));  // Reader exposes them by kind; runtime ignores unknown kinds at consumption.
    }
}
