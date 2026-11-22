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

    // Decals group into texture arrays by PackingLayout equality, so any difference Pack() would
    // trample has to compare unequal. These all share one texture size on purpose -- the
    // same-resolution case a dimensions-only key failed to separate.

    TEST_F(DecalTextureArrayTests, PackingLayout_EqualityIncludesEveryPackedProperty)
    {
        DecalTextureArray::PackingLayout layout;
        for (auto& map : layout.m_maps)
        {
            map.m_size = RHI::Size(1024, 1024, 1);
            map.m_format = RHI::Format::BC7_UNORM;
            map.m_mipLevels = 11;
        }

        const auto identical = layout;
        EXPECT_EQ(layout, identical);

        auto differentFormat = layout;
        differentFormat.m_maps[DecalMapType_Diffuse].m_format = RHI::Format::BC1_UNORM;
        EXPECT_NE(layout, differentFormat);

        auto differentMipCount = layout;
        differentMipCount.m_maps[DecalMapType_Diffuse].m_mipLevels = 1;
        EXPECT_NE(layout, differentMipCount);

        auto withoutNormal = layout;
        withoutNormal.m_maps[DecalMapType_Normal] = DecalTextureArray::PackingLayout::MapLayout{};
        EXPECT_NE(layout, withoutNormal);
    }

    // [GFX TODO][ATOM-5915] Add more comprehensive tests here involving packing StreamingImages
}
