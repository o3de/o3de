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

namespace AZ::Render::GradientGI
{
    inline constexpr uint32_t MinFaceResolution = 4;
    inline constexpr uint32_t MaxFaceResolution = 256;

    inline uint32_t ClampFaceResolution(uint32_t value)
    {
        return AZStd::clamp(value, MinFaceResolution, MaxFaceResolution);
    }

    //! Round within the supported range, clamping before integer conversion. NaN maps to the minimum.
    inline uint32_t RoundAndClampFaceResolution(float value)
    {
        if (!(value >= static_cast<float>(MinFaceResolution)))
        {
            return MinFaceResolution;
        }

        if (value >= static_cast<float>(MaxFaceResolution))
        {
            return MaxFaceResolution;
        }

        return static_cast<uint32_t>(AZStd::lround(value));
    }

    //! Snap to the nearest supported power of two, choosing the lower value on ties.
    //! Avoids observed black output from some non-power-of-two StreamingImage uploads.
    inline uint32_t SnapCpuFaceResolutionToPow2(uint32_t value)
    {
        const uint32_t clamped = ClampFaceResolution(value);
        uint32_t lower = MinFaceResolution;
        while ((lower << 1) <= clamped)
        {
            lower <<= 1;
        }

        const uint32_t upper = lower << 1;
        uint32_t nearest = upper;
        if (clamped - lower <= upper - clamped)
        {
            nearest = lower;
        }

        return ClampFaceResolution(nearest);
    }

    //! Fall back to CPU generation when writable cubemap allocation is unavailable.
    inline GradientGIFeatureProcessorInterface::UpdateMode ResolveUpdateMode(
        GradientGIFeatureProcessorInterface::UpdateMode requested,
        bool gpuComputeSupported)
    {
        using UpdateMode = GradientGIFeatureProcessorInterface::UpdateMode;

        if (requested == UpdateMode::Dynamic && !gpuComputeSupported)
        {
            return UpdateMode::Static;
        }

        return requested;
    }

    //! Tracks owners in activation order so editor/play overlap does not disable a surviving component.
    //! The newest owner wins. Controllers restore their configuration when promoted.
    class OwnerRegistry
    {
    public:
        //! Make this owner current, moving an existing registration rather than duplicating it.
        void Add(const AZ::EntityId& owner)
        {
            Remove(owner);
            m_owners.push_back(owner);
        }

        //! Remove an owner. Unknown IDs are ignored.
        void Remove(const AZ::EntityId& owner)
        {
            const auto it = AZStd::find(m_owners.begin(), m_owners.end(), owner);
            if (it != m_owners.end())
            {
                m_owners.erase(it);
            }
        }

        bool Empty() const
        {
            return m_owners.empty();
        }

        //! Current owner, or an invalid ID when empty.
        AZ::EntityId Current() const
        {
            if (m_owners.empty())
            {
                return AZ::EntityId();
            }

            return m_owners.back();
        }

        size_t Count() const
        {
            return m_owners.size();
        }

        //! Registered owners, oldest first.
        const AZStd::vector<AZ::EntityId>& Entities() const
        {
            return m_owners;
        }

        void Clear()
        {
            m_owners.clear();
        }

    private:
        AZStd::vector<AZ::EntityId> m_owners;
    };

    //! Disable the layer when the texture does not match the requested 2D/cubemap mapping.
    inline bool IsTextureLayerEnabled(bool hasTexture, bool cubeMappingRequested, bool textureIsCube)
    {
        return hasTexture && (cubeMappingRequested == textureIsCube);
    }

    //! Replace backslashes and strip leading separators. Empty or all-separator input returns an empty path.
    inline AZStd::string NormalizeAssetPath(AZStd::string_view rawPath)
    {
        AZStd::string path(rawPath);
        AZStd::replace(path.begin(), path.end(), '\\', '/');
        const size_t firstReal = path.find_first_not_of('/');
        if (firstReal == AZStd::string::npos)
        {
            path.clear();
        }
        else
        {
            path.erase(0, firstReal);
        }
        return path;
    }

    //! Coalesce CPU resolution edits to limit tiled-image allocation churn, a workaround for reported DX12 crashes.
    class ResolutionDebounce
    {
    public:
        ResolutionDebounce() = default;
        explicit ResolutionDebounce(uint8_t settleFrameThreshold)
            : m_threshold(settleFrameThreshold)
        {
        }

        //! Request an already-clamped resolution. Only a different value restarts the settle window.
        void Request(uint32_t value)
        {
            if (value != m_pending)
            {
                m_pending = value;
                m_settleFrames = 0;
            }

            m_changePending = (m_pending != m_committed);
        }

        //! Commit on the call after the settle-frame count is reached. Otherwise return nullopt.
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

        //! Apply a value immediately and cancel any pending request.
        void Commit(uint32_t value)
        {
            m_committed = value;
            m_pending = value;
            m_changePending = false;
            m_settleFrames = 0;
        }

        uint32_t Committed() const
        {
            return m_committed;
        }

        uint32_t Pending() const
        {
            return m_pending;
        }

        bool HasPendingCommit() const
        {
            return m_changePending;
        }

        uint8_t SettleFrames() const
        {
            return m_settleFrames;
        }

        uint8_t Threshold() const
        {
            return m_threshold;
        }

    public:
        static constexpr uint8_t DefaultSettleFrames = 3;

    private:
        uint32_t m_committed = 0;
        uint32_t m_pending = 0;
        uint8_t m_settleFrames = 0;
        uint8_t m_threshold = DefaultSettleFrames;
        bool m_changePending = false;
    };

} // namespace AZ::Render::GradientGI
