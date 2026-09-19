/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/GradientGI/GradientGIFeatureProcessorInterface.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/math.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <cstdint>

// =============================================================================================
// GradientGI pure logic helpers
//
// These are the decision/state pieces that drove the GradientGI instabilities (rapid resolution
// scrubbing, mode fallbacks, path/value normalisation). They are deliberately free of any RHI /
// RPI / scene dependency so they can be unit-tested in isolation, and so the feature processor,
// compute pass, and editor controller all share one source of truth.
// =============================================================================================

namespace AZ::Render::GradientGI
{
    // =========================================================================================
    // Face resolution (cubemap face size in pixels)
    // =========================================================================================

    inline constexpr uint32_t MinFaceResolution = 4;
    inline constexpr uint32_t MaxFaceResolution = 256;

    //! Clamp an integer face resolution into the supported [4..256] range.
    inline uint32_t ClampFaceResolution(uint32_t value)
    {
        return AZStd::clamp(value, MinFaceResolution, MaxFaceResolution);
    }

    //! Script Canvas / Lua hand resolution in as a Number (floating point). Round to the nearest
    //! whole pixel, then clamp into range. Negative or NaN-free out-of-range inputs clamp cleanly.
    inline uint32_t RoundAndClampFaceResolution(float value)
    {
        const int rounded = static_cast<int>(AZStd::lround(value));
        const int clamped = AZStd::clamp(
            rounded, static_cast<int>(MinFaceResolution), static_cast<int>(MaxFaceResolution));
        return static_cast<uint32_t>(clamped);
    }

    //! Snap a CPU/Static face size to the nearest power of two in [4..256].
    //!
    //! O3DE's runtime StreamingImage GPU upload renders certain non-power-of-two cubemap face sizes
    //! black -- an observed dead band around 91..127 at R16G16B16A16, where the per-face subresource
    //! crosses 64KB yet isn't power-of-two/alignment-clean. The gradient is smooth low-frequency
    //! ambient light, so snapping the CPU face size to a power of two is visually lossless and avoids
    //! the quirk. GPU/Dynamic mode is unaffected -- it writes an AttachmentImage from a compute pass,
    //! never a StreamingImage -- so only the CPU build path uses this.
    inline uint32_t SnapCpuFaceResolutionToPow2(uint32_t value)
    {
        const uint32_t clamped = ClampFaceResolution(value);
        uint32_t lower = MinFaceResolution; // 4 is already a power of two
        while ((lower << 1) <= clamped)
        {
            lower <<= 1;
        }
        const uint32_t upper = lower << 1;
        const uint32_t nearest = (clamped - lower <= upper - clamped) ? lower : upper;
        return ClampFaceResolution(nearest);
    }

    // =========================================================================================
    // Update mode fallback (GPU/Dynamic -> CPU/Static)
    // =========================================================================================

    //! GPU/Dynamic mode requires compute UAV cubemap support. When that is unavailable the feature
    //! falls back to CPU/Static; every other request is honoured as-is.
    inline GradientGIFeatureProcessorInterface::UpdateMode ResolveUpdateMode(
        GradientGIFeatureProcessorInterface::UpdateMode requested, bool gpuComputeSupported)
    {
        using UpdateMode = GradientGIFeatureProcessorInterface::UpdateMode;
        return (requested == UpdateMode::Dynamic && !gpuComputeSupported) ? UpdateMode::Static : requested;
    }

    // =========================================================================================
    // Ownership arbitration
    // =========================================================================================

    //! Tracks every component that currently wants the (scene-wide, singleton) GradientGI feature
    //! processor running. The most recently enabled entity drives it; when that entity leaves, an
    //! earlier one is promoted rather than the feature switching off.
    //!
    //! A single "current owner" is not enough, for two reasons seen in practice:
    //!  * Game mode transitions interleave. The same prefab can exist twice -- an editor-only copy
    //!    and a spawned runtime copy -- and the copy that is shutting down must not switch off the
    //!    copy that has just taken over.
    //!  * Duplicating a GradientGI entity makes the duplicate the owner while the original is still
    //!    alive. Deleting the duplicate then issues a perfectly legitimate disable, and with a
    //!    single owner that shut the feature down and left the scene black until something happened
    //!    to re-activate the original.
    //!
    //! Note: the feature processor holds one set of gradient parameters, not one per owner. After a
    //! promotion those parameters are whatever the departing owner last pushed, until the survivor
    //! pushes again. For duplicates (identical configuration) that is invisible; for two genuinely
    //! different components the survivor's colours are stale until its next edit or activation.
    class OwnerRegistry
    {
    public:
        //! Register an owner, or move an existing one to the front of the queue.
        void Add(const AZ::EntityId& owner)
        {
            Remove(owner);
            m_owners.push_back(owner);
        }

        //! Deregister an owner. Unknown entities are ignored, which is what makes a stale disable
        //! from an already-superseded component harmless.
        void Remove(const AZ::EntityId& owner)
        {
            const auto it = AZStd::find(m_owners.begin(), m_owners.end(), owner);
            if (it != m_owners.end())
            {
                m_owners.erase(it);
            }
        }

        //! True when nobody wants the feature running any more.
        bool Empty() const { return m_owners.empty(); }

        //! The entity currently driving the feature; invalid when empty.
        AZ::EntityId Current() const { return m_owners.empty() ? AZ::EntityId() : m_owners.back(); }

        size_t Count() const { return m_owners.size(); }

        //! Every registered owner, oldest first. Used to name them when reporting duplicates.
        const AZStd::vector<AZ::EntityId>& Entities() const { return m_owners; }

        void Clear() { m_owners.clear(); }

    private:
        AZStd::vector<AZ::EntityId> m_owners; // most recently enabled last
    };

    // =========================================================================================
    // Texture layer fallback (detail / specular)
    // =========================================================================================

    //! A detail/specular texture layer only contributes when a texture is present AND -- when Cube
    //! mapping is requested -- the texture is actually a cubemap. Otherwise the layer falls back to
    //! "off" and the gradient (or, for specular, the diffuse gradient) shows through unmodified.
    inline bool IsTextureLayerEnabled(bool hasTexture, bool cubeMappingRequested, bool textureIsCube)
    {
        return hasTexture && (!cubeMappingRequested || textureIsCube);
    }

    // =========================================================================================
    // Asset path normalisation
    // =========================================================================================

    //! Normalise an asset path to the catalog's product-path convention: unify separators to
    //! forward slashes and strip leading slashes so the result is catalog-relative. This lets a
    //! manually typed string path and a script-variable path resolve through the same lookup.
    //! An all-slash or empty input yields "" -- erase(pos,count) is bounded (pos is always 0,
    //! count clamps to the remaining length), so there is no out-of-range and no UB.
    inline AZStd::string NormalizeAssetPath(AZStd::string_view rawPath)
    {
        AZStd::string path(rawPath);
        AZStd::replace(path.begin(), path.end(), '\\', '/');
        const size_t firstReal = path.find_first_not_of('/');
        path.erase(0, (firstReal == AZStd::string::npos) ? path.size() : firstReal);
        return path;
    }

    // =========================================================================================
    // Resolution scrub debounce
    // =========================================================================================

    //! Coalesces rapid resolution requests into a single committed value once the request has held
    //! steady for a number of frames. A CPU/Static resolution change rebuilds a different-sized
    //! tiled StreamingImage; doing that on every frame of a slider scrub churns the DX12 streaming
    //! pool and crashes the RHI. Buffering until the value settles means only the final size is
    //! ever built. Pure state machine -- no engine dependency -- so the behaviour is unit-testable.
    class ResolutionScrubThrottle
    {
    public:
        static constexpr uint8_t DefaultSettleFrames = 3; // ~50ms @60fps after the slider is released

        ResolutionScrubThrottle() = default;
        explicit ResolutionScrubThrottle(uint8_t settleFrameThreshold)
            : m_threshold(settleFrameThreshold)
        {
        }

        //! Register a requested (already-clamped) value. A genuine move of the target restarts the
        //! settle window; an unrelated repeat of the same target leaves the window running.
        void Request(uint32_t value)
        {
            if (value != m_pending)
            {
                m_pending = value;
                m_settleFrames = 0;
            }
            m_changePending = (m_pending != m_committed);
        }

        //! Advance one frame. Returns the settled value (and marks it committed) once a pending
        //! change has held steady for the threshold; otherwise returns nullopt.
        AZStd::optional<uint32_t> AdvanceFrame()
        {
            if (!m_changePending)
            {
                return AZStd::nullopt;
            }
            if (m_settleFrames < m_threshold)
            {
                ++m_settleFrames;
                return AZStd::nullopt;
            }
            m_committed = m_pending;
            m_changePending = false;
            m_settleFrames = 0;
            return m_committed;
        }

        //! Force the baseline to a known value with no pending change. Used for the immediate-apply
        //! paths: the first push at activation, and Dynamic mode (which never debounces).
        void Commit(uint32_t value)
        {
            m_committed = value;
            m_pending = value;
            m_changePending = false;
            m_settleFrames = 0;
        }

        uint32_t Committed() const { return m_committed; }
        uint32_t Pending() const { return m_pending; }
        bool HasPendingCommit() const { return m_changePending; }
        uint8_t SettleFrames() const { return m_settleFrames; }
        uint8_t Threshold() const { return m_threshold; }

    private:
        uint32_t m_committed = 0;
        uint32_t m_pending = 0;
        uint8_t m_settleFrames = 0;
        uint8_t m_threshold = DefaultSettleFrames;
        bool m_changePending = false;
    };

} // namespace AZ::Render::GradientGI
