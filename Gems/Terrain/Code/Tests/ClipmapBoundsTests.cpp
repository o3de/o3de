/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <gmock/gmock.h>

#include <TerrainRenderer/ClipmapBounds.h>
#include <TerrainRenderer/Aabb2i.h>

#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>

namespace UnitTest
{
    class ClipmapBoundsTests
        : public testing::Test
    {
    public:
        void CheckTransformRegionFullBounds(const Terrain::ClipmapBoundsDescriptor& desc);
    };

    void ClipmapBoundsTests::CheckTransformRegionFullBounds(const Terrain::ClipmapBoundsDescriptor& desc)
    {
        Terrain::ClipmapBounds bounds(desc);

        AZ::Aabb worldBounds = bounds.GetWorldBounds();
        float worldBoundsSize = worldBounds.GetXExtent();

        auto output = bounds.TransformRegion(worldBounds);
        ASSERT_EQ(output.size(), 4);

        AZ::Vector2 boundary = AZ::Vector2(
            floorf(worldBounds.GetMax().GetX() / worldBoundsSize),
            floorf(worldBounds.GetMax().GetY() / worldBoundsSize)
        ) * worldBoundsSize;

        Terrain::Vector2i localMax = {
            aznumeric_cast<int32_t>(AZStd::lround(desc.m_worldSpaceCenter.GetX() / desc.m_clipmapToWorldScale)),
            aznumeric_cast<int32_t>(AZStd::lround(desc.m_worldSpaceCenter.GetY() / desc.m_clipmapToWorldScale))
        };
        localMax += aznumeric_cast<int32_t>(desc.m_size / 2ul);

        int32_t intSize = int32_t(desc.m_size);
        Terrain::Vector2i localBoundary = {
            ((localMax.m_x % intSize) + intSize) % intSize,
            ((localMax.m_y % intSize) + intSize) % intSize
        };
        
        // Check each quadrant returned
        Terrain::ClipmapBoundsRegionList expected;
        expected.resize(4);

        expected.at(0).m_localAabb = Terrain::Aabb2i({localBoundary.m_x, localBoundary.m_y}, {intSize, intSize});
        expected.at(0).m_worldAabb = AZ::Aabb::CreateFromMinMaxValues(
            worldBounds.GetMin().GetX(), worldBounds.GetMin().GetY(), 0.0f,
            boundary.GetX(), boundary.GetY(), 0.0f);
        
        expected.at(1).m_localAabb = Terrain::Aabb2i({0, localBoundary.m_y}, {localBoundary.m_x, intSize});
        expected.at(1).m_worldAabb = AZ::Aabb::CreateFromMinMaxValues(
            boundary.GetX(), worldBounds.GetMin().GetY(), 0.0f,
            worldBounds.GetMax().GetX(), boundary.GetY(), 0.0f);
        
        expected.at(2).m_localAabb = Terrain::Aabb2i({localBoundary.m_x, 0}, {intSize, localBoundary.m_y});
        expected.at(2).m_worldAabb = AZ::Aabb::CreateFromMinMaxValues(
            worldBounds.GetMin().GetX(), boundary.GetY(), 0.0f,
            boundary.GetX(), worldBounds.GetMax().GetY(), 0.0f);

        expected.at(3).m_localAabb = Terrain::Aabb2i({ 0, 0 }, { localBoundary.m_x, localBoundary.m_y });
        expected.at(3).m_worldAabb = AZ::Aabb::CreateFromMinMaxValues(
            boundary.GetX(), boundary.GetY(), 0.0f,
            worldBounds.GetMax().GetX(), worldBounds.GetMax().GetY(), 0.0f);

        EXPECT_THAT(output, ::testing::UnorderedElementsAreArray(expected));
    }

    TEST_F(ClipmapBoundsTests, Construction)
    {
        Terrain::ClipmapBoundsDescriptor desc;
        Terrain::ClipmapBounds bounds(desc);
    }
    
    TEST_F(ClipmapBoundsTests, BasicTransform)
    {
        // Create clipmap around 0.0, so it's perfectly divided into 4 quadrants
        Terrain::ClipmapBoundsDescriptor desc;
        desc.m_worldSpaceCenter = AZ::Vector2(0.0f, 0.0f);
        desc.m_clipmapUpdateMultiple = 0;
        desc.m_clipmapToWorldScale = 1.0f;
        desc.m_size = 1024;
        Terrain::ClipmapBounds bounds(desc);

        auto output = bounds.TransformRegion(AZ::Aabb::CreateFromMinMaxValues(-512.0f, -512.0f, 0.0f, 512.0f, 512.0f, 0.0f));

        ASSERT_EQ(output.size(), 4);

        // Check each quadrant returned
        EXPECT_EQ(output.at(0).m_localAabb, Terrain::Aabb2i({512, 512}, {1024, 1024}));
        EXPECT_TRUE(output.at(0).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(-512.0f, -512.0f, 0.0f, 0.0f, 0.0f, 0.0f)));
        EXPECT_EQ(output.at(1).m_localAabb, Terrain::Aabb2i({0, 512}, {512, 1024}));
        EXPECT_TRUE(output.at(1).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(0.0f, -512.0f, 0.0f, 512.0f, 0.0f, 0.0f)));
        EXPECT_EQ(output.at(2).m_localAabb, Terrain::Aabb2i({512, 0}, {1024, 512}));
        EXPECT_TRUE(output.at(2).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(-512.0f, 0.0f, 0.0f, 0.0f, 512.0f, 0.0f)));
        EXPECT_EQ(output.at(3).m_localAabb, Terrain::Aabb2i({0, 0}, {512, 512}));
        EXPECT_TRUE(output.at(3).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 0.0f, 512.0f, 512.0f, 0.0f)));
    }

    TEST_F(ClipmapBoundsTests, ScaledTransform)
    {
        // Create clipmap around 0.0, so it's perfectly divided into 4 quadrants, but half-scale
        Terrain::ClipmapBoundsDescriptor desc;
        desc.m_worldSpaceCenter = AZ::Vector2(0.0f, 0.0f);
        desc.m_clipmapUpdateMultiple = 0;
        desc.m_clipmapToWorldScale = 0.5f;
        desc.m_size = 1024;
        Terrain::ClipmapBounds bounds(desc);

        auto output = bounds.TransformRegion(AZ::Aabb::CreateFromMinMaxValues(-256.0f, -256.0f, 0.0f, 256.0f, 256.0f, 0.0f));

        ASSERT_EQ(output.size(), 4);

        // Check each quadrant returned
        EXPECT_EQ(output.at(0).m_localAabb, Terrain::Aabb2i({512, 512}, {1024, 1024}));
        EXPECT_TRUE(output.at(0).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(-256.0f, -256.0f, 0.0f, 0.0f, 0.0f, 0.0f)));
        EXPECT_EQ(output.at(1).m_localAabb, Terrain::Aabb2i({0, 512}, {512, 1024}));
        EXPECT_TRUE(output.at(1).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(0.0f, -256.0f, 0.0f, 256.0f, 0.0f, 0.0f)));
        EXPECT_EQ(output.at(2).m_localAabb, Terrain::Aabb2i({512, 0}, {1024, 512}));
        EXPECT_TRUE(output.at(2).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(-256.0f, 0.0f, 0.0f, 0.0f, 256.0f, 0.0f)));
        EXPECT_EQ(output.at(3).m_localAabb, Terrain::Aabb2i({0, 0}, {512, 512}));
        EXPECT_TRUE(output.at(3).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 0.0f, 256.0f, 256.0f, 0.0f)));
    }

    TEST_F(ClipmapBoundsTests, ComplexTransformsFullBounds)
    {
        // Check 4 different clipmaps - one in completely positive space, one in negative space, and two straddling the axis

        // Clipmap in negative space
        {
            Terrain::ClipmapBoundsDescriptor desc;
            desc.m_worldSpaceCenter = AZ::Vector2(-1234.0f, -5432.0f);
            desc.m_clipmapUpdateMultiple = 0;
            desc.m_clipmapToWorldScale = 0.75f;
            desc.m_size = 512;
            CheckTransformRegionFullBounds(desc);
        }
        
        // Clipmap in positive space
        {
            Terrain::ClipmapBoundsDescriptor desc;
            desc.m_worldSpaceCenter = AZ::Vector2(1234.0f, 5432.0f);
            desc.m_clipmapUpdateMultiple = 0;
            desc.m_clipmapToWorldScale = 1.25f;
            desc.m_size = 1024;
            CheckTransformRegionFullBounds(desc);
        }
        
        // Clipmap on x axis
        {
            Terrain::ClipmapBoundsDescriptor desc;
            desc.m_worldSpaceCenter = AZ::Vector2(1234.0f, -100.0f);
            desc.m_clipmapUpdateMultiple = 0;
            desc.m_clipmapToWorldScale = 1.5f;
            desc.m_size = 256;
            CheckTransformRegionFullBounds(desc);
        }
        // Clipmap on y axis
        {
            Terrain::ClipmapBoundsDescriptor desc;
            desc.m_worldSpaceCenter = AZ::Vector2(-100.0f, 5432.0f);
            desc.m_clipmapUpdateMultiple = 0;
            desc.m_clipmapToWorldScale = 1.0f;
            desc.m_size = 2048;
            CheckTransformRegionFullBounds(desc);
        }
    }

    TEST_F(ClipmapBoundsTests, TransformSmallBounds)
    {
        // Create clipmap around 0.0, so it's perfectly divided into 4 quadrants
        Terrain::ClipmapBoundsDescriptor desc;
        desc.m_worldSpaceCenter = AZ::Vector2(0.0f, 0.0f);
        desc.m_clipmapUpdateMultiple = 0;
        desc.m_clipmapToWorldScale = 1.0f;
        desc.m_size = 1024;
        Terrain::ClipmapBounds bounds(desc);

        {
            // Single quadrant positive

            AZ::Aabb smallArea = AZ::Aabb::CreateFromMinMaxValues(
                10.0f, 10.0f, 0.0f, 50.0f, 50.0f, 0.0f
            );

            auto output = bounds.TransformRegion(smallArea);

            ASSERT_EQ(output.size(), 1);
            EXPECT_EQ(output.at(0).m_localAabb, Terrain::Aabb2i({10, 10}, {50, 50}));
            EXPECT_TRUE(output.at(0).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(10.0f, 10.0f, 0.0f, 50.0f, 50.0f, 0.0f)));
        }

        {
            // Single quadrant negative

            AZ::Aabb smallArea = AZ::Aabb::CreateFromMinMaxValues(
                -50.0f, -50.0f, 0.0f, -10.0f, -10.0f, 0.0f
            );

            auto output = bounds.TransformRegion(smallArea);

            ASSERT_EQ(output.size(), 1);
            EXPECT_EQ(output.at(0).m_localAabb, Terrain::Aabb2i({974, 974}, {1014, 1014}));
            EXPECT_TRUE(output.at(0).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(-50.0f, -50.0f, 0.0f, -10.0f, -10.0f, 0.0f)));
        }

        {
            // 2 quadrant positive
            
            AZ::Aabb smallArea = AZ::Aabb::CreateFromMinMaxValues(
                10.0f, -10.0f, 0.0f, 50.0f, 50.0f, 0.0f
            );

            auto output = bounds.TransformRegion(smallArea);

            ASSERT_EQ(output.size(), 2);
            EXPECT_EQ(output.at(0).m_localAabb, Terrain::Aabb2i({10, 1014}, {50, 1024}));
            EXPECT_TRUE(output.at(0).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(10.0f, -10.0f, 0.0f, 50.0f, 0.0f, 0.0f)));
            EXPECT_EQ(output.at(1).m_localAabb, Terrain::Aabb2i({10, 0}, {50, 50}));
            EXPECT_TRUE(output.at(1).m_worldAabb.IsClose(AZ::Aabb::CreateFromMinMaxValues(10.0f, 0.0f, 0.0f, 50.0f, 50.0f, 0.0f)));
        }
    }

    TEST_F(ClipmapBoundsTests, MarginReducesUpdates)
    {
        // With a margin defined, the bounds should only trigger updates when the camera moves outside the margins

        // Create clipmap around 0.0, so it's perfectly divided into 4 quadrants
        Terrain::ClipmapBoundsDescriptor desc;
        desc.m_worldSpaceCenter = AZ::Vector2(0.0f, 0.0f);
        desc.m_clipmapUpdateMultiple = 16;
        desc.m_clipmapToWorldScale = 1.0f;
        desc.m_size = 1024;
        Terrain::ClipmapBounds bounds(desc);

        // center moved forward to 10, still within margin
        auto output1 = bounds.UpdateCenter(AZ::Vector2(10.0f, 10.0f));
        EXPECT_EQ(output1.size(), 0);
        // center moved forwrd to 20, beyond margin, triggers update
        auto output2 = bounds.UpdateCenter(AZ::Vector2(20.0f, 20.0f));
        EXPECT_GT(output2.size(), 0);
        // center moved back to 10, still within margin
        auto output3 = bounds.UpdateCenter(AZ::Vector2(10.0f, 10.0f));
        EXPECT_EQ(output3.size(), 0);
        // center moved back to 0, still within margin (on edge)
        auto output4 = bounds.UpdateCenter(AZ::Vector2(0.0f, 0.0f));
        EXPECT_EQ(output4.size(), 0);
        // center moved back to -10, beyond margin, triggers update
        auto output5 = bounds.UpdateCenter(AZ::Vector2(-10.0f, -10.0f));
        EXPECT_GT(output5.size(), 0);
    }
    
    TEST_F(ClipmapBoundsTests, CenterMovementUpdates)
    {
        // Create clipmap around 0.0, so it's perfectly divided into 4 quadrants
        Terrain::ClipmapBoundsDescriptor desc;
        desc.m_worldSpaceCenter = AZ::Vector2(0.0f, 0.0f);
        desc.m_clipmapUpdateMultiple = 16;
        desc.m_clipmapToWorldScale = 1.0f;
        desc.m_size = 1024;
        Terrain::ClipmapBounds bounds(desc);

        {
            AZ::Aabb untouchedRegion = AZ::Aabb::CreateNull();
            auto output = bounds.UpdateCenter(AZ::Vector2(20.0f, 20.0f), &untouchedRegion);
            ASSERT_EQ(output.size(), 4);

            // Instead of checking bounds directly, do several checks to make sure the bounds are appropriate. Since
            // the center moved just outside the margin along the diagonal, we should expect two edges to be updated
            // that are the width of the margin.

            // 1. The number of pixels updated in the bounds should be two sides of margin width
            float pixelsCovered = 0;
            for (auto& region : output)
            {
                // Note: GetSurfaceArea() returns the area of all 6 sides of the aabb. With a Z extent of 0, that
                //   means that only the top and bottom will be counted, so we need to multiply by 0.5.
                pixelsCovered += region.m_worldAabb.GetSurfaceArea() * 0.5f;
            }

            // Two edges of margin * size, minus the overlap in the corner.
            const uint32_t updateMultiple = desc.m_clipmapUpdateMultiple;
            float expectedCoverage = updateMultiple * desc.m_size * 2.0f - updateMultiple * updateMultiple;
            EXPECT_NEAR(pixelsCovered, expectedCoverage, 0.0001f);

            // 2. The untouched region area should match what's expected
            float untouchedRegionArea = untouchedRegion.GetSurfaceArea() * 0.5f;
            float expectedUntouchedRegionSide = aznumeric_cast<float>(desc.m_size - desc.m_clipmapUpdateMultiple);
            float expectedUntouchedRegionArea = expectedUntouchedRegionSide * expectedUntouchedRegionSide;
            EXPECT_NEAR(untouchedRegionArea, expectedUntouchedRegionArea, 0.0001f);

            // 3. All of the update regions should be inside the world bounds of the clipmap
            AZ::Aabb worldBounds = bounds.GetWorldBounds();
            for (auto& region : output)
            {
                EXPECT_EQ(region.m_worldAabb.GetClamped(worldBounds), region.m_worldAabb);
            }

            // 4. The untouched region should also be inside the world bounds of the clipmap;
            EXPECT_EQ(untouchedRegion.GetClamped(worldBounds), untouchedRegion);

            // 5. None of the update regions should overlap each other or the untouched region

            // push the untouched region on the vector to make comparisons easier
            output.push_back(Terrain::ClipmapBoundsRegion({untouchedRegion, Terrain::Aabb2i({}) }));
            for (uint32_t i = 0; i < output.size(); ++i)
            {
                const AZ::Aabb boundsToCheck = output.at(i).m_worldAabb;
                for (uint32_t j = i + 1; j < output.size(); ++j)
                {
                    // AZ::Aabb::Overlaps() counts touching edges as overlapping, so we need a strict version
                    auto strictOverlaps = [](const AZ::Aabb& aabb1, const AZ::Aabb& aabb2) -> bool
                    {
                        return aabb1.GetMin().IsLessThan(aabb2.GetMax()) &&
                            aabb1.GetMax().IsGreaterThan(aabb2.GetMin());
                    };
                    EXPECT_FALSE(strictOverlaps(boundsToCheck, output.at(j).m_worldAabb));
                }
            }

        }

    }

    // This test is to ensure clipmap update compute shader receives 6 regions at most.
    TEST_F(ClipmapBoundsTests, MaxUpdateRegionTest)
    {
        // The initial clipmap is divided into 4 parts.
        // By traversing the 11x11 grid, all possible overlapping cases can be covered.
        for (int32_t i = -5; i <= 5; ++i)
        {
            for (int32_t j = -5; j <= 5; ++j)
            {
                Terrain::ClipmapBoundsDescriptor desc;
                desc.m_worldSpaceCenter = AZ::Vector2(0.0f, 0.0f);
                desc.m_clipmapUpdateMultiple = 0;
                desc.m_clipmapToWorldScale = 1.0f;
                desc.m_size = 1024;
                Terrain::ClipmapBounds bounds(desc);

                auto list = bounds.UpdateCenter(AZ::Vector2(256.0f * i, 256.0f * j));

                uint32_t size = aznumeric_cast<uint32_t>(list.size());
                EXPECT_LE(size, Terrain::ClipmapBounds::MaxUpdateRegions);
            }
        }
    }

}
