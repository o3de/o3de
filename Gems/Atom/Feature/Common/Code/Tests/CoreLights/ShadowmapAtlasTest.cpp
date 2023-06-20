/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <random>
#include <algorithm>
#include <iterator>

#include <AzTest/AzTest.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Math/Random.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <CoreLights/ShadowmapAtlas.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Render;

    class ShadowmapAtlasTests
        : public ::testing::Test
    {
    public:
        // This randomize the given array with the given random seed.
        template<typename T>
        void RandomizeArray(AZStd::vector<T>& array, unsigned int seed)
        {
            std::mt19937 random(seed);
            std::shuffle(array.begin(), array.end(), random);
        }

        using ShadowmapPixels = AZStd::vector<AZStd::vector<bool>>;

        // This checks the given area (x0, y0)-(x0+size, y0+size)
        // has not been occupied by the other shadowmap already.
        void TestNotFilledYet(
            ShadowmapPixels& pixels,
            uint32_t x0,
            uint32_t y0,
            ShadowmapSize size)
        {
            for (uint32_t y = 0; y < static_cast<uint32_t>(size); ++y)
            {
                for (uint32_t x = 0; y < static_cast<uint32_t>(size); ++y)
                {
                    EXPECT_FALSE(pixels[x0 + x][y0 + y]); // test non-occupation
                    pixels[x0 + x][y0 + y] = true; // occupy here
                }
            }
        }

        // This creates an array of ShadowPixels of given size
        // initialized all pixels in false (not occupied yet).
        AZStd::vector<ShadowmapPixels> CreateShadowPixels(uint16_t arraySliceCount, ShadowmapSize shadowmapSize)
        {
            AZStd::vector<ShadowmapPixels> pixelArray(arraySliceCount);
            const uint32_t width = static_cast<uint32_t>(shadowmapSize);
            for (uint16_t sliceIndex = 0; sliceIndex < arraySliceCount; ++sliceIndex)
            {
                pixelArray[sliceIndex].resize(width);
                for (uint32_t yIndex = 0; yIndex < width; ++yIndex)
                {
                    pixelArray[sliceIndex][yIndex].resize(width, false);
                }
            }
            return pixelArray;
        }
    };

    // no shadowmap
    TEST_F(ShadowmapAtlasTests, Empty)
    {
        ShadowmapAtlas atlas;
        atlas.Initialize();
        atlas.Finalize();

        // If no shadowmap is added, it returns 1
        // since an image resource have to be created for such a case.
        EXPECT_EQ(1, atlas.GetArraySliceCount());
        EXPECT_EQ(ShadowmapSize::None, atlas.GetBaseShadowmapSize());
    }

    // single shadowmap of size 256
    TEST_F(ShadowmapAtlasTests, SingleSmallSize)
    {
        ShadowmapAtlas atlas;
        atlas.Initialize();
        atlas.SetShadowmapSize(0, ShadowmapSize::Size256);
        atlas.Finalize();

        const ShadowmapAtlas::Origin origin = atlas.GetOrigin(0);
        EXPECT_EQ(1, atlas.GetArraySliceCount());
        EXPECT_EQ(ShadowmapSize::Size256, atlas.GetBaseShadowmapSize());
        EXPECT_EQ(0, origin.m_arraySlice);
        EXPECT_EQ(0, origin.m_originInSlice[0]);
        EXPECT_EQ(0, origin.m_originInSlice[1]);
    }

    // single shadowmap of size 2048
    TEST_F(ShadowmapAtlasTests, SingleLargeSize)
    {
        ShadowmapAtlas atlas;
        atlas.Initialize();
        atlas.SetShadowmapSize(0, ShadowmapSize::Size2048);
        atlas.Finalize();

        const ShadowmapAtlas::Origin origin = atlas.GetOrigin(0);
        EXPECT_EQ(1, atlas.GetArraySliceCount());
        EXPECT_EQ(ShadowmapSize::Size2048, atlas.GetBaseShadowmapSize());
        EXPECT_EQ(0, origin.m_arraySlice);
        EXPECT_EQ(0, origin.m_originInSlice[0]);
        EXPECT_EQ(0, origin.m_originInSlice[1]);
    }

    // multiple shadowmaps of size 1024 in random order
    TEST_F(ShadowmapAtlasTests, MultipleMiddleSize)
    {
        constexpr size_t ShadowmapCount = 20;
        AZStd::vector<size_t> indices(ShadowmapCount);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            indices[index] = index;
        }
        const unsigned int seed = 0;
        RandomizeArray(indices, seed);

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (size_t index : indices)
        {
            atlas.SetShadowmapSize(index, ShadowmapSize::Size1024);
        }
        atlas.Finalize();

        EXPECT_EQ(ShadowmapCount, atlas.GetArraySliceCount());
        EXPECT_EQ(ShadowmapSize::Size1024, atlas.GetBaseShadowmapSize());
        AZStd::unordered_set<uint16_t> arraySlices;
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            const ShadowmapAtlas::Origin origin = atlas.GetOrigin(index);
            arraySlices.insert(origin.m_arraySlice);
            EXPECT_EQ(0, origin.m_originInSlice[0]);
            EXPECT_EQ(0, origin.m_originInSlice[1]);
        }
        EXPECT_EQ(ShadowmapCount, arraySlices.size());
    }

    // multiple shadowmaps of sizes in ascending order
    TEST_F(ShadowmapAtlasTests, AscendingSizes)
    {
        const AZStd::vector<ShadowmapSize> sizes = { {
                ShadowmapSize::None,
                ShadowmapSize::Size256,
                ShadowmapSize::Size512,
                ShadowmapSize::Size1024,
                ShadowmapSize::Size2048
            } };
        // [slice:0] 1 x 2048x2048,
        // [slice:1] 1 x 1024x1024 + 1 x 512x512 + 1 x 256x256.
        const uint16_t ExpectedArraySliceCount = 2;

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (size_t index = 0; index < sizes.size(); ++index)
        {
            atlas.SetShadowmapSize(index, sizes[index]);
        }
        atlas.Finalize();

        ASSERT_EQ(ExpectedArraySliceCount, atlas.GetArraySliceCount());
        ASSERT_EQ(ShadowmapSize::Size2048, atlas.GetBaseShadowmapSize());
        AZStd::vector<ShadowmapPixels> pixelArray = CreateShadowPixels(ExpectedArraySliceCount, ShadowmapSize::Size2048);
        for (uint32_t index = 0; index < sizes.size(); ++index)
        {
            if (sizes[index] != ShadowmapSize::None)
            {
                const ShadowmapAtlas::Origin origin = atlas.GetOrigin(index);
                if (sizes[index] == ShadowmapSize::Size2048)
                {
                    EXPECT_EQ(0, origin.m_arraySlice);
                    EXPECT_EQ(0, origin.m_originInSlice[0]);
                    EXPECT_EQ(0, origin.m_originInSlice[1]);
                }
                else
                {
                    EXPECT_EQ(1, origin.m_arraySlice);
                }
                TestNotFilledYet(
                    pixelArray[origin.m_arraySlice],
                    origin.m_originInSlice[0],
                    origin.m_originInSlice[1],
                    sizes[index]);
            }
        }
    }

    // multiple shadowmaps of sizes in descending order
    TEST_F(ShadowmapAtlasTests, DescendingSizes)
    {
        const AZStd::vector<ShadowmapSize> sizes = { {
                ShadowmapSize::Size2048,
                ShadowmapSize::Size1024,
                ShadowmapSize::Size512,
                ShadowmapSize::Size256,
                ShadowmapSize::None
            } };
        // [slice:0] 1 x 2048x2048,
        // [slice:1] 1 x 1024x1024 + 1 x 512x512 + 1 x 256x256.
        const uint16_t ExpectedArraySliceCount = 2;

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (size_t index = 0; index < sizes.size(); ++index)
        {
            atlas.SetShadowmapSize(index, sizes[index]);
        }
        atlas.Finalize();

        ASSERT_EQ(ExpectedArraySliceCount, atlas.GetArraySliceCount());
        ASSERT_EQ(ShadowmapSize::Size2048, atlas.GetBaseShadowmapSize());
        AZStd::vector<ShadowmapPixels> pixelArray = CreateShadowPixels(ExpectedArraySliceCount, ShadowmapSize::Size2048);
        for (uint32_t index = 0; index < sizes.size(); ++index)
        {
            if (sizes[index] != ShadowmapSize::None)
            {
                const ShadowmapAtlas::Origin origin = atlas.GetOrigin(index);
                if (sizes[index] == ShadowmapSize::Size2048)
                {
                    EXPECT_EQ(0, origin.m_arraySlice);
                    EXPECT_EQ(0, origin.m_originInSlice[0]);
                    EXPECT_EQ(0, origin.m_originInSlice[1]);
                }
                else
                {
                    EXPECT_EQ(1, origin.m_arraySlice);
                }
                TestNotFilledYet(
                    pixelArray[origin.m_arraySlice],
                    origin.m_originInSlice[0],
                    origin.m_originInSlice[1],
                    sizes[index]);
            }
        }
    }

    // multiple shadowmaps of sizes in random order
    TEST_F(ShadowmapAtlasTests, SizesInRadomOrder)
    {
        AZStd::vector<ShadowmapSize> sizes = { {
                ShadowmapSize::None,
                ShadowmapSize::Size256,
                ShadowmapSize::Size512,
                ShadowmapSize::Size1024,
                ShadowmapSize::Size2048
            } };
        // [slice:0] 1 x 2048x2048,
        // [slice:1] 1 x 1024x1024 + 1 x 512x512 + 1 x 256x256.
        const uint16_t ExpectedArraySliceCount = 2;

        AZStd::vector<size_t> indices(sizes.size());
        for (size_t index = 0; index < sizes.size(); ++index)
        {
            indices[index] = index;
        }
        const unsigned int seed = 0;
        RandomizeArray(indices, seed);
        RandomizeArray(sizes, seed + 1);

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (size_t index : indices)
        {
            atlas.SetShadowmapSize(index, sizes[index]);
        }
        atlas.Finalize();

        ASSERT_EQ(ExpectedArraySliceCount, atlas.GetArraySliceCount());
        ASSERT_EQ(ShadowmapSize::Size2048, atlas.GetBaseShadowmapSize());
        AZStd::vector<ShadowmapPixels> pixelArray = CreateShadowPixels(ExpectedArraySliceCount, ShadowmapSize::Size2048);
        for (uint32_t index = 0; index < sizes.size(); ++index)
        {
            if (sizes[index] == ShadowmapSize::None)
            {
                continue;
            }

            const ShadowmapAtlas::Origin origin = atlas.GetOrigin(index);
            if (sizes[index] == ShadowmapSize::Size2048)
            {
                EXPECT_EQ(0, origin.m_arraySlice);
                EXPECT_EQ(0, origin.m_originInSlice[0]);
                EXPECT_EQ(0, origin.m_originInSlice[1]);
            }
            else
            {
                EXPECT_EQ(1, origin.m_arraySlice);
            }
            TestNotFilledYet(
                pixelArray[origin.m_arraySlice],
                origin.m_originInSlice[0],
                origin.m_originInSlice[1],
                sizes[index]);
        }
    }

    // multiple shadowmaps of single 2048 size and multiple 256 size.
    TEST_F(ShadowmapAtlasTests, SizesALargeAndSmalls)
    {
        constexpr size_t ShadowmapCount = 50;
        // 1 x 2048x2048 + 49 x 256x256
        //   --> [slice:0] 1 x 2048x2048,
        //       [slice:1] 49 x 256x256.
        constexpr uint16_t ExpectedArraySliceCount = 2;

        AZStd::vector<ShadowmapSize> sizes(ShadowmapCount, ShadowmapSize::Size256);
        sizes[0] = ShadowmapSize::Size2048;
        AZStd::vector<size_t> indices(ShadowmapCount);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            indices[index] = index;
        }
        const unsigned int seed = 0;
        RandomizeArray(indices, seed);
        RandomizeArray(sizes, seed + 1);

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (size_t index : indices)
        {
            atlas.SetShadowmapSize(index, sizes[index]);
        }
        atlas.Finalize();

        ASSERT_EQ(ExpectedArraySliceCount, atlas.GetArraySliceCount());
        ASSERT_EQ(ShadowmapSize::Size2048, atlas.GetBaseShadowmapSize());
        AZStd::vector<ShadowmapPixels> pixelArray = CreateShadowPixels(ExpectedArraySliceCount, ShadowmapSize::Size2048);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            const ShadowmapAtlas::Origin origin = atlas.GetOrigin(index);
            TestNotFilledYet(
                pixelArray[origin.m_arraySlice],
                origin.m_originInSlice[0],
                origin.m_originInSlice[1],
                sizes[index]);
        }
    }
    
    // multiple shadowmaps of single 2048 size and multiple 512 size.
    TEST_F(ShadowmapAtlasTests, SizesALargeAndMiddles)
    {
        constexpr size_t ShadowmapCount = 50;
        // 1 x 2048x2048 + 49 x 512x512
        //   --> [slice:0] 1 x 2048x2048,
        //       [slice:1] 16 x 512x512,
        //       [slice:2] 16 x 512x512,
        //       [slice:3] 16 x 512x512,
        //       [slice:4]  1 x 512x512.
        constexpr uint16_t ExpectedArraySliceCount = 5;

        AZStd::vector<ShadowmapSize> sizes(ShadowmapCount, ShadowmapSize::Size512);
        sizes[0] = ShadowmapSize::Size2048;
        AZStd::vector<size_t> indices(ShadowmapCount);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            indices[index] = index;
        }
        const unsigned int seed = 0;
        RandomizeArray(indices, seed);
        RandomizeArray(sizes, seed + 1);

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (size_t index : indices)
        {
            atlas.SetShadowmapSize(index, sizes[index]);
        }
        atlas.Finalize();

        ASSERT_EQ(ExpectedArraySliceCount, atlas.GetArraySliceCount());
        ASSERT_EQ(ShadowmapSize::Size2048, atlas.GetBaseShadowmapSize());
        AZStd::vector<ShadowmapPixels> pixelArray = CreateShadowPixels(ExpectedArraySliceCount, ShadowmapSize::Size2048);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            const ShadowmapAtlas::Origin origin = atlas.GetOrigin(index);
            TestNotFilledYet(
                pixelArray[origin.m_arraySlice],
                origin.m_originInSlice[0],
                origin.m_originInSlice[1],
                sizes[index]);
        }
    }
    
    // multiple shadowmaps of several sizes in random order.
    TEST_F(ShadowmapAtlasTests, VariousSizes)
    {
        constexpr size_t ShadowmapCount = 2 + 3 + 10 + 20;
        // 2 x 2048x2048 + 3 x 1024x1024 + 10 x 512x512 + 20 x 256x256
        //   --> [slice:0] 1 x 2048x2048,
        //       [slice:1] 1 x 2048x2048,
        //       [slice:2] 3 x 1024x1024 +  4 x 512x512,
        //       [slice:3] 6 x 512x512   + 20 x 256x256.
        constexpr uint16_t ExpectedArraySliceCount = 4;

        AZStd::vector<ShadowmapSize> sizes;
        sizes.reserve(ShadowmapCount);
        for (int index = 0; index < 2; ++index)
        {
            sizes.push_back(ShadowmapSize::Size2048);
        }
        for (int index = 0; index < 3; ++index)
        {
            sizes.push_back(ShadowmapSize::Size1024);
        }
        for (int index = 0; index < 10; ++index)
        {
            sizes.push_back(ShadowmapSize::Size512);
        }
        for (int index = 0; index < 20; ++index)
        {
            sizes.push_back(ShadowmapSize::Size256);
        }

        AZStd::vector<size_t> indices(ShadowmapCount);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            indices[index] = index;
        }
        const unsigned int seed = 0;
        RandomizeArray(indices, seed);
        RandomizeArray(sizes, seed + 1);

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (size_t index : indices)
        {
            atlas.SetShadowmapSize(index, sizes[index]);
        }
        atlas.Finalize();

        ASSERT_EQ(ExpectedArraySliceCount, atlas.GetArraySliceCount());
        ASSERT_EQ(ShadowmapSize::Size2048, atlas.GetBaseShadowmapSize());
        AZStd::vector<ShadowmapPixels> pixelArray = CreateShadowPixels(ExpectedArraySliceCount, ShadowmapSize::Size2048);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            const ShadowmapAtlas::Origin origin = atlas.GetOrigin(index);
            TestNotFilledYet(
                pixelArray[origin.m_arraySlice],
                origin.m_originInSlice[0],
                origin.m_originInSlice[1],
                sizes[index]);
        }
    }

    // multiple shadowmaps of several sizes not so large in random order.
    TEST_F(ShadowmapAtlasTests, VariousSizesNotSoLarge)
    {
        constexpr size_t ShadowmapCount = 5 + 10 + 40;
        // 5 x 1024x1024 + 10 x 512x512 + 40 x 256x256
        //   --> [slice:0] 1 x 1024x1024,
        //       [slice:1] 1 x 1024x1024,
        //       [slice:2] 1 x 1024x1024,
        //       [slice:3] 1 x 1024x1024,
        //       [slice:4] 1 x 1024x1024,
        //       [slice:5] 4 x 512x512,
        //       [slice:6] 4 x 512x512,
        //       [slice:7] 2 x 512x512 + 8 x 256x256,
        //       [slice:8] 16 x 256x256,
        //       [slice:9] 16 x 256x256.
        constexpr uint16_t ExpectedArraySliceCount = 10;

        AZStd::vector<ShadowmapSize> sizes;
        sizes.reserve(ShadowmapCount);
        for (int index = 0; index < 5; ++index)
        {
            sizes.push_back(ShadowmapSize::Size1024);
        }
        for (int index = 0; index < 10; ++index)
        {
            sizes.push_back(ShadowmapSize::Size512);
        }
        for (int index = 0; index < 40; ++index)
        {
            sizes.push_back(ShadowmapSize::Size256);
        }

        AZStd::vector<size_t> indices(ShadowmapCount);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            indices[index] = index;
        }
        const unsigned int seed = 0;
        RandomizeArray(indices, seed);
        RandomizeArray(sizes, seed + 1);

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (size_t index : indices)
        {
            atlas.SetShadowmapSize(index, sizes[index]);
        }
        atlas.Finalize();

        ASSERT_EQ(ExpectedArraySliceCount, atlas.GetArraySliceCount());
        ASSERT_EQ(ShadowmapSize::Size1024, atlas.GetBaseShadowmapSize());
        AZStd::vector<ShadowmapPixels> pixelArray = CreateShadowPixels(ExpectedArraySliceCount, ShadowmapSize::Size2048);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            const ShadowmapAtlas::Origin origin = atlas.GetOrigin(index);
            TestNotFilledYet(
                pixelArray[origin.m_arraySlice],
                origin.m_originInSlice[0],
                origin.m_originInSlice[1],
                sizes[index]);
        }
    }

    // Index Table for no shadowmap
    TEST_F(ShadowmapAtlasTests, IndexEmpty)
    {
        using IndexNode = ShadowmapAtlas::ShadowmapIndexNode;
        ShadowmapAtlas atlas;
        atlas.Initialize();
        atlas.Finalize();

        AZStd::vector<IndexNode> table = atlas.GetShadowmapIndexTable();
        EXPECT_EQ(1, table.size());
    }

    // Index Table for 1 shadowmap
    TEST_F(ShadowmapAtlasTests, IndexSingle)
    {
        using IndexNode = ShadowmapAtlas::ShadowmapIndexNode;
        ShadowmapAtlas atlas;
        atlas.Initialize();
        atlas.SetShadowmapSize(0, ShadowmapSize::Size1024);
        atlas.Finalize();

        AZStd::vector<IndexNode> table = atlas.GetShadowmapIndexTable();
        EXPECT_EQ(1, table.size());
        EXPECT_EQ(0, table[0].m_nextTableOffset);
        EXPECT_EQ(0, table[0].m_shadowmapIndex);
    }

    // Index Table for multiple shadowmaps of the same size.
    TEST_F(ShadowmapAtlasTests, IndexMultipleShadowmapsSameSize)
    {
        using IndexNode = ShadowmapAtlas::ShadowmapIndexNode;
        constexpr uint32_t ShadowmapCount = 10;
        AZStd::vector<uint32_t> indices(ShadowmapCount);
        for (uint32_t index = 0; index < ShadowmapCount; ++index)
        {
            indices[index] = index;
        }
        const unsigned int seed = 0;
        RandomizeArray(indices, seed);

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (uint32_t index : indices)
        {
            atlas.SetShadowmapSize(index, ShadowmapSize::Size1024);
        }
        atlas.Finalize();

        AZStd::vector<IndexNode> table = atlas.GetShadowmapIndexTable();

        EXPECT_EQ(ShadowmapCount, table.size());
        for (uint32_t index = 0; index < ShadowmapCount; ++index)
        {
            const uint32_t slice = atlas.GetOrigin(index).m_arraySlice;
            EXPECT_EQ(0, table[slice].m_nextTableOffset);
            EXPECT_EQ(index, table[slice].m_shadowmapIndex);
        }
    }

    // Index Table for 10 shadowmaps of 2 sizes.
    TEST_F(ShadowmapAtlasTests, IndexTableMultipleShadowmapsTwoSizes)
    {
        using IndexNode = ShadowmapAtlas::ShadowmapIndexNode;

        constexpr size_t ShadowmapCount = 10;
        // 1 x 2048x2048 + 9 x 512x512
        //   --> [slice:0]  1 x 2048x2048,
        //       [slice:1]  9 x 512x512.
        AZStd::vector<ShadowmapSize> sizes(ShadowmapCount, ShadowmapSize::Size512);
        sizes[0] = ShadowmapSize::Size2048;
        AZStd::vector<size_t> indices(ShadowmapCount);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            indices[index] = index;
        }
        const unsigned int seed = 0;
        RandomizeArray(indices, seed);

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (size_t index : indices)
        {
            atlas.SetShadowmapSize(index, sizes[index]);
        }
        atlas.Finalize();

        AZStd::vector<IndexNode> table = atlas.GetShadowmapIndexTable();

        constexpr size_t ExpectedTableSize =
            2 /* slice size */
            + 4 /* 1024 size node count */
            + 4 + 4 + 4; /* 512 size node count */
        EXPECT_EQ(ExpectedTableSize, table.size());

        // shadowmap of size 2048
        EXPECT_EQ(0, table[0].m_nextTableOffset);
        EXPECT_EQ(0, table[0].m_shadowmapIndex);

        // shadowmaps of size 512
        for (uint32_t index = 1; index < ShadowmapCount; ++index)
        {
            uint32_t tableOffset = table[1].m_nextTableOffset;
            EXPECT_LE(2, tableOffset);

            ShadowmapAtlas::Origin origin = atlas.GetOrigin(index);
            uint8_t isRight = origin.m_originInSlice[0] >= 1024;
            uint8_t isDown = origin.m_originInSlice[1] >= 1024;
            uint8_t digit = isRight + isDown * 2;

            tableOffset = table[tableOffset + digit].m_nextTableOffset;
            EXPECT_NE(0, tableOffset);
            isRight = (origin.m_originInSlice[0] % 1024) >= 512;
            isDown = (origin.m_originInSlice[1] % 1024) >= 512;
            digit = isRight + isDown * 2;
            EXPECT_EQ(0, table[tableOffset + digit].m_nextTableOffset);
            EXPECT_EQ(index, table[tableOffset + digit].m_shadowmapIndex);
        }
    }

    // Index Table for 20 shadowmaps of 3 sizes.
    TEST_F(ShadowmapAtlasTests, IndexTableMultipleShadowmapsThreeSizes)
    {
        using IndexNode = ShadowmapAtlas::ShadowmapIndexNode;

        constexpr size_t ShadowmapCount = 20;
        // 1 x 2048x2048 + 6 x 1024x1024 + 13 x 512x512
        //   --> [slice:0]  1 x 2048x2048,
        //       [slice:1]  4 x 1024x1024,
        //       [slice:2]  2 x 1024x1024 + 8 x 512x512,
        //       [slice:3]  5 x 512x512,
        AZStd::vector<ShadowmapSize> sizes(ShadowmapCount, ShadowmapSize::Size512);
        sizes[0] = ShadowmapSize::Size2048;
        for (size_t index = 0; index < 6; ++index)
        {
            sizes[1 + index] = ShadowmapSize::Size1024;
        }
        AZStd::vector<size_t> indices(ShadowmapCount);
        for (size_t index = 0; index < ShadowmapCount; ++index)
        {
            indices[index] = index;
        }
        const unsigned int seed = 0;
        RandomizeArray(indices, seed);
        RandomizeArray(sizes, seed + 1);

        ShadowmapAtlas atlas;
        atlas.Initialize();
        for (size_t index : indices)
        {
            atlas.SetShadowmapSize(index, sizes[index]);
        }
        atlas.Finalize();

        AZStd::vector<IndexNode> table = atlas.GetShadowmapIndexTable();

        constexpr size_t ExpectedTableSize =
            4 /* slice size */
            + 4 + 4 + 4 /* 1024 size table count */
            + 4 + 4 + 4 + 4; /* 512 size table count */
        EXPECT_EQ(ExpectedTableSize, table.size());

        // shadowmap of size 2048
        EXPECT_EQ(0, table[0].m_nextTableOffset);
        for (uint32_t index = 0; index < ShadowmapCount; ++index)
        {
            if (sizes[index] == ShadowmapSize::Size2048)
            {
                EXPECT_EQ(index, table[0].m_shadowmapIndex);
                break;
            }
        }

        // shadowmaps of size 1024
        size_t count1024 = 0;
        EXPECT_EQ(4, table[1].m_nextTableOffset);
        for (uint32_t index = 4; index < 4 + 4; ++index)
        {
            EXPECT_EQ(0, table[index].m_nextTableOffset);
            const int32_t lightIndex = table[index].m_shadowmapIndex;
            EXPECT_EQ(sizes[lightIndex], ShadowmapSize::Size1024) << "LightIndex:" << lightIndex;
            ++count1024;
        }
        EXPECT_EQ(8, table[2].m_nextTableOffset);
        for (uint32_t index = 8; index < 8 + 4; ++index)
        {
            if (index < 8 + 2)
            {
                EXPECT_EQ(0, table[index].m_nextTableOffset);
                const int32_t lightIndex = table[index].m_shadowmapIndex;
                EXPECT_EQ(sizes[lightIndex], ShadowmapSize::Size1024) << "LightIndex:" << lightIndex;
                ++count1024;
            }
            else
            {
                EXPECT_LT(0, table[index].m_nextTableOffset);
                EXPECT_EQ(~0, table[index].m_shadowmapIndex);
            }
        }
        EXPECT_EQ(6, count1024);

        // shadowmaps of size 512
        size_t count512 = 0;
        auto checkSize512 = [&](uint32_t startIndex)
        {
            for (uint32_t index = startIndex; index < startIndex + 4; ++index)
            {
                EXPECT_EQ(0, table[index].m_nextTableOffset) << "index:" << index;
                const int32_t lightIndex = table[index].m_shadowmapIndex;
                if (lightIndex >= 0)
                {
                    EXPECT_EQ(sizes[lightIndex], ShadowmapSize::Size512) << "LightIndex:" << lightIndex;
                    ++count512;
                }
            }
        };

        // slice count is 4, count of size 1024 subtable is 3,
        // so base offset of size 512 is 4 + 3 * 4 = 16.
        EXPECT_EQ(16, table[10].m_nextTableOffset);
        checkSize512(16);
        EXPECT_EQ(20, table[11].m_nextTableOffset);
        checkSize512(20);

        EXPECT_EQ(12, table[3].m_nextTableOffset);
        EXPECT_EQ(24, table[12].m_nextTableOffset);
        checkSize512(24);
        EXPECT_EQ(28, table[13].m_nextTableOffset);
        checkSize512(28);

        EXPECT_EQ(0, table[14].m_nextTableOffset);
        EXPECT_EQ(0, table[15].m_nextTableOffset);
        EXPECT_EQ(13, count512);
    }
} // namespace UnitTest
