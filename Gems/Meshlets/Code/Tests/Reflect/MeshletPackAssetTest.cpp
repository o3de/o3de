/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>
#include <Meshlets/Reflect/MeshletPackAsset.h>
#include <Meshlets/Reflect/MeshletPackWriter.h>

namespace UnitTest
{
    using namespace AZ::Meshlets;

    TEST(MeshletPackAsset, LoadsValidPackAndExposesHeader)
    {
        // Build a minimal valid pack with just a PackHeader section.
        MeshletPackWriter w;
        w.BeginPack();
        PackHeaderRecord h{};
        h.m_meshCount = 0;
        h.m_maxVerticesPerCluster = 64;
        h.m_maxTrianglesPerCluster = 64;
        h.m_coneWeight = 0.5f;
        w.AddSection(SectionKind::PackHeader, &h, sizeof(h));
        AZStd::vector<AZ::u8> bytes;
        ASSERT_TRUE(w.End(bytes));

        MeshletPackAsset asset;
        ASSERT_TRUE(asset.LoadFromBuffer(AZStd::move(bytes)));
        const PackHeaderRecord* read = asset.GetPackHeader();
        ASSERT_NE(nullptr, read);
        EXPECT_EQ(64u, read->m_maxVerticesPerCluster);
    }

    TEST(MeshletPackAsset, RejectsMalformedBytes)
    {
        AZStd::vector<AZ::u8> garbage(64, 0xFF);
        MeshletPackAsset asset;
        EXPECT_FALSE(asset.LoadFromBuffer(AZStd::move(garbage)));
        // After failure, GetPackHeader returns null.
        EXPECT_EQ(nullptr, asset.GetPackHeader());
    }

    TEST(MeshletPackAsset, AssetExtensionStringMatchesSpec)
    {
        EXPECT_STREQ("azmeshletpack", MeshletPackAsset::Extension);
    }
}
