/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientGlobalIllumination/GradientGlobalIlluminationImageUtils.h>
#include <Atom/Feature/GradientGlobalIllumination/GradientGlobalIlluminationLogic.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <limits>

namespace UnitTest
{
    using namespace AZ::Render;
    using namespace AZ::Render::GradientGlobalIllumination;
    using UpdateMode = GradientGlobalIlluminationFeatureProcessorInterface::UpdateMode;

    class GradientGlobalIlluminationLogicFixture
        : public LeakDetectionFixture
    {
    };

    TEST_F(GradientGlobalIlluminationLogicFixture, ClampFaceResolution_HoldsRangeBounds)
    {
        EXPECT_EQ(ClampFaceResolution(64), 64u);
        EXPECT_EQ(ClampFaceResolution(MinFaceResolution), MinFaceResolution);
        EXPECT_EQ(ClampFaceResolution(MaxFaceResolution), MaxFaceResolution);
        EXPECT_EQ(ClampFaceResolution(0), MinFaceResolution); // below floor
        EXPECT_EQ(ClampFaceResolution(2), MinFaceResolution); // below floor
        EXPECT_EQ(ClampFaceResolution(1000), MaxFaceResolution); // above ceiling
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, SnapCpuFaceResolutionToPow2_AvoidsTheBlackBand)
    {
        // Existing powers of two remain unchanged.
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(4), 4u);
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(64), 64u);
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(128), 128u);
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(256), 256u);

        // Non-power-of-two requests snap to the nearest supported size.
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(91), 64u); // nearer 64
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(127), 128u); // nearer 128
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(100), 128u);
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(200), 256u);

        // Check range and power-of-two invariants across the full supported interval.
        for (uint32_t v = MinFaceResolution; v <= MaxFaceResolution; ++v)
        {
            const uint32_t snapped = SnapCpuFaceResolutionToPow2(v);
            EXPECT_GE(snapped, MinFaceResolution);
            EXPECT_LE(snapped, MaxFaceResolution);
            EXPECT_EQ(snapped & (snapped - 1), 0u) << "not a power of two for input " << v;
        }
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, RoundAndClampFaceResolution_RoundsThenClamps)
    {
        EXPECT_EQ(RoundAndClampFaceResolution(63.4f), 63u);
        EXPECT_EQ(RoundAndClampFaceResolution(63.6f), 64u);
        EXPECT_EQ(RoundAndClampFaceResolution(127.5f), 128u); // round half away from zero
        EXPECT_EQ(RoundAndClampFaceResolution(64.0f), 64u);

        // Negative requests must not wrap when converted to an unsigned size.
        EXPECT_EQ(RoundAndClampFaceResolution(-5.0f), MinFaceResolution);
        EXPECT_EQ(RoundAndClampFaceResolution(3.2f), MinFaceResolution);
        EXPECT_EQ(RoundAndClampFaceResolution(9000.0f), MaxFaceResolution);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, RoundAndClampFaceResolution_HandlesNonFiniteAndExtremeValues)
    {
        EXPECT_EQ(RoundAndClampFaceResolution(std::numeric_limits<float>::max()), MaxFaceResolution);
        EXPECT_EQ(RoundAndClampFaceResolution(std::numeric_limits<float>::lowest()), MinFaceResolution);
        EXPECT_EQ(RoundAndClampFaceResolution(std::numeric_limits<float>::infinity()), MaxFaceResolution);
        EXPECT_EQ(RoundAndClampFaceResolution(-std::numeric_limits<float>::infinity()), MinFaceResolution);
        EXPECT_EQ(RoundAndClampFaceResolution(std::numeric_limits<float>::quiet_NaN()), MinFaceResolution);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, ResolveUpdateMode_DynamicFallsBackWhenGpuUnsupported)
    {
        EXPECT_EQ(ResolveUpdateMode(UpdateMode::Dynamic, /*gpuSupported*/ false), UpdateMode::Static);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, ResolveUpdateMode_DynamicHonouredWhenGpuSupported)
    {
        EXPECT_EQ(ResolveUpdateMode(UpdateMode::Dynamic, /*gpuSupported*/ true), UpdateMode::Dynamic);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, ResolveUpdateMode_StaticAlwaysStatic)
    {
        EXPECT_EQ(ResolveUpdateMode(UpdateMode::Static, false), UpdateMode::Static);
        EXPECT_EQ(ResolveUpdateMode(UpdateMode::Static, true), UpdateMode::Static);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, IsTextureLayerEnabled_DisabledWhenNoTexture)
    {
        EXPECT_FALSE(IsTextureLayerEnabled(/*hasTexture*/ false, /*cubeRequested*/ false, /*textureIsCube*/ false));
        EXPECT_FALSE(IsTextureLayerEnabled(false, true, true));
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, IsTextureLayerEnabled_NonCubeMappingRequires2DTexture)
    {
        EXPECT_TRUE(IsTextureLayerEnabled(true, /*cubeRequested*/ false, /*textureIsCube*/ false));
        EXPECT_FALSE(IsTextureLayerEnabled(true, false, true));
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, IsTextureLayerEnabled_CubeMappingRequiresCubeTexture)
    {
        EXPECT_FALSE(IsTextureLayerEnabled(true, /*cubeRequested*/ true, /*textureIsCube*/ false));
        EXPECT_TRUE(IsTextureLayerEnabled(true, true, true));
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, NormalizeAssetPath_UnifiesSeparators)
    {
        EXPECT_EQ(NormalizeAssetPath("textures\\foo.streamingimage"), "textures/foo.streamingimage");
        EXPECT_EQ(NormalizeAssetPath("a\\b/c"), "a/b/c");
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, NormalizeAssetPath_StripsLeadingSlashes)
    {
        EXPECT_EQ(NormalizeAssetPath("/leading"), "leading");
        EXPECT_EQ(NormalizeAssetPath("///multi"), "multi");
        EXPECT_EQ(NormalizeAssetPath("\\\\winlead"), "winlead");
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, NormalizeAssetPath_AllSeparatorsOrEmptyYieldEmpty)
    {
        EXPECT_EQ(NormalizeAssetPath("/"), "");
        EXPECT_EQ(NormalizeAssetPath("///"), "");
        EXPECT_EQ(NormalizeAssetPath(""), "");
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, NormalizeAssetPath_LeavesCleanRelativePathUnchanged)
    {
        EXPECT_EQ(NormalizeAssetPath("envhdri/photo.streamingimage"), "envhdri/photo.streamingimage");
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, ResolutionDebounce_DefaultsToThreeFrameSettle)
    {
        EXPECT_EQ(ResolutionDebounce::DefaultSettleFrames, 3);
        ResolutionDebounce debounce;
        EXPECT_EQ(debounce.Threshold(), 3);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, ResolutionDebounce_CommitSetsBaselineWithNoPendingChange)
    {
        ResolutionDebounce debounce;
        debounce.Commit(64);
        EXPECT_EQ(debounce.Committed(), 64u);
        EXPECT_EQ(debounce.Pending(), 64u);
        EXPECT_FALSE(debounce.HasPendingCommit());
        EXPECT_FALSE(debounce.AdvanceFrame().has_value());
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, ResolutionDebounce_RequestingCommittedValueIsNoOp)
    {
        ResolutionDebounce debounce;
        debounce.Commit(64);
        debounce.Request(64);
        EXPECT_FALSE(debounce.HasPendingCommit());
        EXPECT_FALSE(debounce.AdvanceFrame().has_value());
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, ResolutionDebounce_StableRequestCommitsOnceAfterSettle)
    {
        ResolutionDebounce debounce(/*threshold*/ 3);
        debounce.Commit(64);
        debounce.Request(128);
        EXPECT_TRUE(debounce.HasPendingCommit());

        EXPECT_FALSE(debounce.AdvanceFrame().has_value());
        EXPECT_FALSE(debounce.AdvanceFrame().has_value());
        EXPECT_FALSE(debounce.AdvanceFrame().has_value());

        const AZStd::optional<uint32_t> committed = debounce.AdvanceFrame();
        ASSERT_TRUE(committed.has_value());
        EXPECT_EQ(*committed, 128u);
        EXPECT_EQ(debounce.Committed(), 128u);
        EXPECT_FALSE(debounce.HasPendingCommit());

        EXPECT_FALSE(debounce.AdvanceFrame().has_value());
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, ResolutionDebounce_ActiveScrubNeverCommitsThenSettlesOnFinalValue)
    {
        ResolutionDebounce debounce(/*threshold*/ 3);
        debounce.Commit(64);

        // Change the request before each frame advance, then allow the final value to settle.
        const uint32_t scrub[] = { 70, 80, 96, 120, 160, 200, 256 };
        for (uint32_t value : scrub)
        {
            debounce.Request(value);
            EXPECT_FALSE(debounce.AdvanceFrame().has_value()) << "must not commit mid-scrub";
        }
        EXPECT_EQ(debounce.Committed(), 64u);

        AZStd::optional<uint32_t> committed;
        for (int frame = 0; frame < 16 && !committed.has_value(); ++frame)
        {
            committed = debounce.AdvanceFrame();
        }
        ASSERT_TRUE(committed.has_value());
        EXPECT_EQ(*committed, 256u); // only the final size is ever applied
        EXPECT_EQ(debounce.Committed(), 256u);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, ResolutionDebounce_ReturningToCommittedValueCancelsPending)
    {
        ResolutionDebounce debounce(/*threshold*/ 3);
        debounce.Commit(64);
        debounce.Request(128);
        EXPECT_TRUE(debounce.HasPendingCommit());

        debounce.Request(64); // moved back to the committed size before it settled
        EXPECT_FALSE(debounce.HasPendingCommit());
        EXPECT_FALSE(debounce.AdvanceFrame().has_value());
        EXPECT_EQ(debounce.Committed(), 64u);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, ResolutionDebounce_RespectsCustomThreshold)
    {
        ResolutionDebounce debounce(/*threshold*/ 1);
        debounce.Commit(10);
        debounce.Request(20);
        EXPECT_FALSE(debounce.AdvanceFrame().has_value()); // one settle frame
        const AZStd::optional<uint32_t> committed = debounce.AdvanceFrame();
        ASSERT_TRUE(committed.has_value());
        EXPECT_EQ(*committed, 20u);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, OwnerRegistry_LatestOwnerDrives)
    {
        OwnerRegistry owners;
        const AZ::EntityId first(0x1111);
        const AZ::EntityId second(0x2222);

        owners.Add(first);
        EXPECT_EQ(owners.Current(), first);

        owners.Add(second);
        EXPECT_EQ(owners.Current(), second);
        EXPECT_EQ(owners.Count(), 2u);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, OwnerRegistry_PromotesEarlierOwnerInsteadOfStopping)
    {
        // Removing the newest owner must restore the surviving one.
        OwnerRegistry owners;
        const AZ::EntityId original(0x1111);
        const AZ::EntityId duplicate(0x2222);

        owners.Add(original);
        owners.Add(duplicate);
        owners.Remove(duplicate);

        EXPECT_FALSE(owners.Empty());
        EXPECT_EQ(owners.Current(), original);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, OwnerRegistry_StaleDisableIsHarmless)
    {
        // Teardown from an earlier activation must not disable its replacement.
        OwnerRegistry owners;
        const AZ::EntityId editorCopy(0x1111);
        const AZ::EntityId spawnedCopy(0x2222);

        owners.Add(editorCopy);
        owners.Remove(spawnedCopy);

        EXPECT_EQ(owners.Count(), 1u);
        EXPECT_EQ(owners.Current(), editorCopy);
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, OwnerRegistry_StopsOnlyWhenTheLastOwnerLeaves)
    {
        OwnerRegistry owners;
        const AZ::EntityId only(0x1234);

        owners.Add(only);
        owners.Remove(only);
        EXPECT_TRUE(owners.Empty());
        EXPECT_FALSE(owners.Current().IsValid());

        owners.Remove(only);
        EXPECT_TRUE(owners.Empty());
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, OwnerRegistry_ReEnableDoesNotDuplicateAnOwner)
    {
        OwnerRegistry owners;
        const AZ::EntityId entity(0x1234);

        owners.Add(entity);
        owners.Add(entity);
        EXPECT_EQ(owners.Count(), 1u);

        owners.Remove(entity);
        EXPECT_TRUE(owners.Empty());
    }

    TEST_F(GradientGlobalIlluminationLogicFixture, Rgba16FloatWriterPreservesHdrValuesAndSupportsUnalignedDestination)
    {
        uint8_t bytes[9]{};
        WriteRgba16FloatPixel(bytes + 1, -1.0f, 0.5f, 2.0f, 1.0f);
        const uint16_t expected[] = { 0xBC00, 0x3800, 0x4000, 0x3C00 };
        EXPECT_EQ(std::memcmp(bytes + 1, expected, sizeof(expected)), 0);
        EXPECT_EQ(bytes[0], 0);
    }
} // namespace UnitTest
