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
        : public UnitTest::AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();
        }

        void TearDown() override
        {
            UnitTest::AllocatorsTestFixture::TearDown();
        }
    };

    TEST_F(DecalTextureArrayTests, TestPackingNothing)
    {
        AZ::Render::DecalTextureArray decalTextureArray;
        decalTextureArray.Pack();
        auto nothing = decalTextureArray.GetPackedTexture(AZ::Render::DecalMapType_Diffuse);
        EXPECT_EQ(nothing, nullptr);
    }

    // [GFX TODO][ATOM-5915] Add more comprehensive tests here involving packing StreamingImages
}
