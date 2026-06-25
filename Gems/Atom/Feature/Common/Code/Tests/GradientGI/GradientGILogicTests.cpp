/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/Feature/GradientGI/GradientGILogic.h>

// =============================================================================================
// GradientGI logic unit tests
//
// These cover the pure decision/state pieces behind the GradientGI feature's known instabilities:
//   * ResolutionScrubThrottle -- the debounce that fixed the DX12 tiled-pool churn crash when the
//     resolution slider is scrubbed (different-sized cubemap allocated every frame).
//   * ResolveUpdateMode       -- the GPU/Dynamic -> CPU/Static fallback when compute is unsupported.
//   * IsTextureLayerEnabled   -- the detail/specular "no texture falls back" decision.
//   * Clamp/Round + NormalizeAssetPath -- value/path normalisation for the scripting API.
//
// The actual GPU compute compositing (specular rendering as diffuse, detail-absent showing pure
// gradient) is shader behaviour and is verified as a behavioural proof, not here.
// =============================================================================================

namespace UnitTest
{
    using namespace AZ::Render;
    using namespace AZ::Render::GradientGI;
    using UpdateMode = GradientGIFeatureProcessorInterface::UpdateMode;

    class GradientGILogicFixture : public LeakDetectionFixture
    {
    };

    // =========================================================================================
    // Face resolution clamp / round
    // =========================================================================================

    TEST_F(GradientGILogicFixture, ClampFaceResolution_HoldsRangeBounds)
    {
        EXPECT_EQ(ClampFaceResolution(64), 64u);
        EXPECT_EQ(ClampFaceResolution(MinFaceResolution), MinFaceResolution);
        EXPECT_EQ(ClampFaceResolution(MaxFaceResolution), MaxFaceResolution);
        EXPECT_EQ(ClampFaceResolution(0), MinFaceResolution);     // below floor
        EXPECT_EQ(ClampFaceResolution(2), MinFaceResolution);     // below floor
        EXPECT_EQ(ClampFaceResolution(1000), MaxFaceResolution);  // above ceiling
    }

    TEST_F(GradientGILogicFixture, SnapCpuFaceResolutionToPow2_AvoidsTheBlackBand)
    {
        // Powers of two pass through unchanged.
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(4), 4u);
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(64), 64u);
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(128), 128u);
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(256), 256u);

        // The observed dead band (91..127 at 16-bit RGBA) snaps out to a safe power of two.
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(91), 64u);   // nearer 64
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(127), 128u); // nearer 128
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(100), 128u);
        EXPECT_EQ(SnapCpuFaceResolutionToPow2(200), 256u);

        // Result is always a power of two within [4..256], whatever the input.
        for (uint32_t v = MinFaceResolution; v <= MaxFaceResolution; ++v)
        {
            const uint32_t snapped = SnapCpuFaceResolutionToPow2(v);
            EXPECT_GE(snapped, MinFaceResolution);
            EXPECT_LE(snapped, MaxFaceResolution);
            EXPECT_EQ(snapped & (snapped - 1), 0u) << "not a power of two for input " << v;
        }
    }

    TEST_F(GradientGILogicFixture, RoundAndClampFaceResolution_RoundsThenClamps)
    {
        // Script Canvas Numbers arrive as float -- round to nearest whole pixel.
        EXPECT_EQ(RoundAndClampFaceResolution(63.4f), 63u);
        EXPECT_EQ(RoundAndClampFaceResolution(63.6f), 64u);
        EXPECT_EQ(RoundAndClampFaceResolution(127.5f), 128u); // round half away from zero
        EXPECT_EQ(RoundAndClampFaceResolution(64.0f), 64u);

        // Out-of-range floats clamp cleanly (negatives must not wrap to a huge unsigned value).
        EXPECT_EQ(RoundAndClampFaceResolution(-5.0f), MinFaceResolution);
        EXPECT_EQ(RoundAndClampFaceResolution(3.2f), MinFaceResolution);
        EXPECT_EQ(RoundAndClampFaceResolution(9000.0f), MaxFaceResolution);
    }

    // =========================================================================================
    // Update mode fallback (GPU/Dynamic -> CPU/Static)
    // =========================================================================================

    TEST_F(GradientGILogicFixture, ResolveUpdateMode_DynamicFallsBackWhenGpuUnsupported)
    {
        EXPECT_EQ(ResolveUpdateMode(UpdateMode::Dynamic, /*gpuSupported*/ false), UpdateMode::Static);
    }

    TEST_F(GradientGILogicFixture, ResolveUpdateMode_DynamicHonouredWhenGpuSupported)
    {
        EXPECT_EQ(ResolveUpdateMode(UpdateMode::Dynamic, /*gpuSupported*/ true), UpdateMode::Dynamic);
    }

    TEST_F(GradientGILogicFixture, ResolveUpdateMode_StaticAlwaysStatic)
    {
        EXPECT_EQ(ResolveUpdateMode(UpdateMode::Static, false), UpdateMode::Static);
        EXPECT_EQ(ResolveUpdateMode(UpdateMode::Static, true), UpdateMode::Static);
    }

    // =========================================================================================
    // Texture layer fallback (no detail / no specular -> gradient shows through)
    // =========================================================================================

    TEST_F(GradientGILogicFixture, IsTextureLayerEnabled_DisabledWhenNoTexture)
    {
        EXPECT_FALSE(IsTextureLayerEnabled(/*hasTexture*/ false, /*cubeRequested*/ false, /*textureIsCube*/ false));
        EXPECT_FALSE(IsTextureLayerEnabled(false, true, true));
    }

    TEST_F(GradientGILogicFixture, IsTextureLayerEnabled_EnabledForNonCubeMapping)
    {
        EXPECT_TRUE(IsTextureLayerEnabled(true, /*cubeRequested*/ false, /*textureIsCube*/ false));
        EXPECT_TRUE(IsTextureLayerEnabled(true, false, true));
    }

    TEST_F(GradientGILogicFixture, IsTextureLayerEnabled_CubeMappingRequiresCubeTexture)
    {
        // Cube mapping requested but a 2D texture supplied -> disabled (don't bind 2D to a cube slot).
        EXPECT_FALSE(IsTextureLayerEnabled(true, /*cubeRequested*/ true, /*textureIsCube*/ false));
        EXPECT_TRUE(IsTextureLayerEnabled(true, true, true));
    }

    // =========================================================================================
    // Asset path normalisation
    // =========================================================================================

    TEST_F(GradientGILogicFixture, NormalizeAssetPath_UnifiesSeparators)
    {
        EXPECT_EQ(NormalizeAssetPath("textures\\foo.streamingimage"), "textures/foo.streamingimage");
        EXPECT_EQ(NormalizeAssetPath("a\\b/c"), "a/b/c");
    }

    TEST_F(GradientGILogicFixture, NormalizeAssetPath_StripsLeadingSlashes)
    {
        EXPECT_EQ(NormalizeAssetPath("/leading"), "leading");
        EXPECT_EQ(NormalizeAssetPath("///multi"), "multi");
        EXPECT_EQ(NormalizeAssetPath("\\\\winlead"), "winlead");
    }

    TEST_F(GradientGILogicFixture, NormalizeAssetPath_AllSeparatorsOrEmptyYieldEmpty)
    {
        // The case Kyler flagged: all-slash / empty input must not be UB -- erase is bounded.
        EXPECT_EQ(NormalizeAssetPath("/"), "");
        EXPECT_EQ(NormalizeAssetPath("///"), "");
        EXPECT_EQ(NormalizeAssetPath(""), "");
    }

    TEST_F(GradientGILogicFixture, NormalizeAssetPath_LeavesCleanRelativePathUnchanged)
    {
        EXPECT_EQ(NormalizeAssetPath("envhdri/photo.streamingimage"), "envhdri/photo.streamingimage");
    }

    // =========================================================================================
    // Resolution scrub debounce
    // =========================================================================================

    TEST_F(GradientGILogicFixture, ScrubThrottle_DefaultsToThreeFrameSettle)
    {
        EXPECT_EQ(ResolutionScrubThrottle::DefaultSettleFrames, 3);
        ResolutionScrubThrottle throttle;
        EXPECT_EQ(throttle.Threshold(), 3);
    }

    TEST_F(GradientGILogicFixture, ScrubThrottle_CommitSetsBaselineWithNoPendingChange)
    {
        ResolutionScrubThrottle throttle;
        throttle.Commit(64);
        EXPECT_EQ(throttle.Committed(), 64u);
        EXPECT_EQ(throttle.Pending(), 64u);
        EXPECT_FALSE(throttle.HasPendingCommit());
        EXPECT_FALSE(throttle.AdvanceFrame().has_value());
    }

    TEST_F(GradientGILogicFixture, ScrubThrottle_RequestingCommittedValueIsNoOp)
    {
        ResolutionScrubThrottle throttle;
        throttle.Commit(64);
        throttle.Request(64);
        EXPECT_FALSE(throttle.HasPendingCommit());
        EXPECT_FALSE(throttle.AdvanceFrame().has_value());
    }

    TEST_F(GradientGILogicFixture, ScrubThrottle_StableRequestCommitsOnceAfterSettle)
    {
        ResolutionScrubThrottle throttle(/*threshold*/ 3);
        throttle.Commit(64);
        throttle.Request(128);
        EXPECT_TRUE(throttle.HasPendingCommit());

        // Held steady: nothing commits until the settle window elapses.
        EXPECT_FALSE(throttle.AdvanceFrame().has_value());
        EXPECT_FALSE(throttle.AdvanceFrame().has_value());
        EXPECT_FALSE(throttle.AdvanceFrame().has_value());

        const AZStd::optional<uint32_t> committed = throttle.AdvanceFrame();
        ASSERT_TRUE(committed.has_value());
        EXPECT_EQ(*committed, 128u);
        EXPECT_EQ(throttle.Committed(), 128u);
        EXPECT_FALSE(throttle.HasPendingCommit());

        // Already committed: no further commits.
        EXPECT_FALSE(throttle.AdvanceFrame().has_value());
    }

    TEST_F(GradientGILogicFixture, ScrubThrottle_ActiveScrubNeverCommitsThenSettlesOnFinalValue)
    {
        ResolutionScrubThrottle throttle(/*threshold*/ 3);
        throttle.Commit(64);

        // Simulate a slider scrub: the value moves every frame, advancing one frame per move.
        const uint32_t scrub[] = { 70, 80, 96, 120, 160, 200, 256 };
        for (uint32_t value : scrub)
        {
            throttle.Request(value);
            EXPECT_FALSE(throttle.AdvanceFrame().has_value()) << "must not commit mid-scrub";
        }
        // The build size stayed put for the whole scrub -- this is what avoids the RHI churn crash.
        EXPECT_EQ(throttle.Committed(), 64u);

        // Slider released: hold the final value and let it settle.
        AZStd::optional<uint32_t> committed;
        for (int frame = 0; frame < 16 && !committed.has_value(); ++frame)
        {
            committed = throttle.AdvanceFrame();
        }
        ASSERT_TRUE(committed.has_value());
        EXPECT_EQ(*committed, 256u);          // only the final size is ever applied
        EXPECT_EQ(throttle.Committed(), 256u);
    }

    TEST_F(GradientGILogicFixture, ScrubThrottle_ReturningToCommittedValueCancelsPending)
    {
        ResolutionScrubThrottle throttle(/*threshold*/ 3);
        throttle.Commit(64);
        throttle.Request(128);
        EXPECT_TRUE(throttle.HasPendingCommit());

        throttle.Request(64); // moved back to the committed size before it settled
        EXPECT_FALSE(throttle.HasPendingCommit());
        EXPECT_FALSE(throttle.AdvanceFrame().has_value());
        EXPECT_EQ(throttle.Committed(), 64u);
    }

    TEST_F(GradientGILogicFixture, ScrubThrottle_RespectsCustomThreshold)
    {
        ResolutionScrubThrottle throttle(/*threshold*/ 1);
        throttle.Commit(10);
        throttle.Request(20);
        EXPECT_FALSE(throttle.AdvanceFrame().has_value()); // one settle frame
        const AZStd::optional<uint32_t> committed = throttle.AdvanceFrame();
        ASSERT_TRUE(committed.has_value());
        EXPECT_EQ(*committed, 20u);
    }

} // namespace UnitTest
