/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>
#include <Atom/Feature/Utils/IndexableList.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <gtest/gtest.h>
#include <AzCore/Math/Random.h>
#include <Decals/DecalTextureArray.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAssetCreator.h>
#include <AzCore/UnitTest/TestTypes.h>


namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Render;

    class DecalTextureArrayTests
        : public UnitTest::LeakDetectionFixture
    {
    };

    TEST_F(DecalTextureArrayTests, TestPackingNothing)
    {
        AZ::Render::DecalTextureArray decalTextureArray;
        decalTextureArray.Pack();
        auto nothing = decalTextureArray.GetPackedTexture(AZ::Render::DecalMapType_Diffuse);
        EXPECT_EQ(nothing, nullptr);
    }

    static DecalTextureArray::PackingLayout MakeUniformLayout(
        uint32_t width, uint32_t height, RHI::Format format, uint16_t mipLevels)
    {
        DecalTextureArray::PackingLayout layout;
        for (int i = 0; i < DecalMapType_Num; ++i)
        {
            layout.m_maps[i].m_size = RHI::Size(width, height, 1);
            layout.m_maps[i].m_format = format;
            layout.m_maps[i].m_mipLevels = mipLevels;
        }
        return layout;
    }

    // Decals group into texture arrays by PackingLayout equality, so any difference Pack() would
    // trample has to compare unequal. These all share one texture size on purpose -- the
    // same-resolution case a dimensions-only key failed to separate.

    TEST_F(DecalTextureArrayTests, PackingLayout_IdenticalLayoutsMatch)
    {
        const auto a = MakeUniformLayout(1024, 1024, RHI::Format::BC7_UNORM, 11);
        const auto b = MakeUniformLayout(1024, 1024, RHI::Format::BC7_UNORM, 11);
        EXPECT_TRUE(a == b);
        EXPECT_FALSE(a != b);
    }

    TEST_F(DecalTextureArrayTests, PackingLayout_SameSizeDifferentFormatDoesNotMatch)
    {
        const auto bc7 = MakeUniformLayout(1024, 1024, RHI::Format::BC7_UNORM, 11);
        const auto bc1 = MakeUniformLayout(1024, 1024, RHI::Format::BC1_UNORM, 11);
        EXPECT_TRUE(bc7 != bc1);
    }

    TEST_F(DecalTextureArrayTests, PackingLayout_SameSizeDifferentMipCountDoesNotMatch)
    {
        const auto fullMips = MakeUniformLayout(1024, 1024, RHI::Format::BC7_UNORM, 11);
        const auto oneMip = MakeUniformLayout(1024, 1024, RHI::Format::BC7_UNORM, 1);
        EXPECT_TRUE(fullMips != oneMip);
    }

    TEST_F(DecalTextureArrayTests, PackingLayout_DifferingOnlyInNormalMapDoesNotMatch)
    {
        // Pack() discards the whole packed normal-map array if any slice is missing one, so adding a
        // decal without a normal map would otherwise strip normals from the rest of its array.
        const auto withNormal = MakeUniformLayout(1024, 1024, RHI::Format::BC7_UNORM, 11);

        auto withoutNormal = withNormal;
        withoutNormal.m_maps[DecalMapType_Normal] = DecalTextureArray::PackingLayout::MapLayout{};

        EXPECT_TRUE(withNormal.m_maps[DecalMapType_Diffuse] == withoutNormal.m_maps[DecalMapType_Diffuse]);
        EXPECT_TRUE(withNormal != withoutNormal);
    }

    // [GFX TODO][ATOM-5915] Add more comprehensive tests here involving packing StreamingImages
}
