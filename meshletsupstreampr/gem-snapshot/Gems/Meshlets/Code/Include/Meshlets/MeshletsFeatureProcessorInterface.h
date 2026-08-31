/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Transform.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace AZ
{
    namespace Meshlets
    {
        //! Public interface to the Meshlets feature processor.
        //!
        //! Designed for cross-gem use: callers (e.g. AtomLyIntegration's MeshComponent
        //! when its "Use Meshlets" toggle is on) interact only with opaque handles, so
        //! they don't pull in MeshletsRenderObject / MeshletsRenderInstance internals.
        //!
        //! Object sharing: AcquireInstance is keyed by ModelAsset id internally. Two
        //! callers acquiring instances of the same model share a single
        //! MeshletsRenderObject (and therefore a single per-frame compute dispatch);
        //! they each get their own ObjectId / DrawPacket.
        class MeshletsFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Meshlets::MeshletsFeatureProcessorInterface,
                "{8B5C2E11-3F4A-4A7E-9E5C-7B1D2A8F6C30}", RPI::FeatureProcessor);

            //! Pack-resolution status. Surfaces in editor diagnostics; mirrors the
            //! impl's PackResolutionStatus values.
            enum class PackResolutionStatus : AZ::u8
            {
                NotChecked = 0,
                Ok,
                NoPack,
                LoadFailed,
                PassesAbsent,
            };

            //! Opaque handle to a meshlet instance. Compare against InvalidInstanceHandle
            //! to detect failure.
            using InstanceHandle = uint64_t;
            static constexpr InstanceHandle InvalidInstanceHandle = ~static_cast<InstanceHandle>(0);

            //! Acquire a new instance for the given model. The first call for a given
            //! model loads/builds the meshlet data; subsequent calls share that work.
            //!
            //! \param modelAsset Must be in a Ready state. Caller retains ownership.
            //! \returns A valid handle on success, InvalidInstanceHandle otherwise.
            virtual InstanceHandle AcquireInstance(const Data::Asset<RPI::ModelAsset>& modelAsset) = 0;

            //! Release an instance previously acquired via AcquireInstance.
            //! Safe to call with InvalidInstanceHandle (no-op).
            virtual void ReleaseInstance(InstanceHandle handle) = 0;

            //! Update the world transform for an instance.
            virtual void SetInstanceTransform(InstanceHandle handle, const AZ::Transform& worldTransform) = 0;

            //! Pack-resolution status. Surfaces in editor diagnostics; mirrors the
            //! impl's PackResolutionStatus values.
            virtual PackResolutionStatus GetPackStatus(const AZ::Data::AssetId& modelAssetId) const = 0;
        };
    } // namespace Meshlets
} // namespace AZ
