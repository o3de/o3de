/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/MorphTargets/MorphTargetInputBuffers.h>

#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/ConstantsData.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace RHI
    {
        class BufferView;
        class PipelineState;
    }

    namespace RPI
    {
        class Buffer;
        class ModelLod;
        class Shader;
        class ShaderResourceGroup;
    }

    namespace Render
    {
        class SkinnedMeshFeatureProcessor;

        //! Holds and manages an RHI DispatchItem for a specific morph target, and the resources that are needed to build and maintain it.
        class MorphTargetDispatchItem
            : private RPI::ShaderReloadNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(MorphTargetDispatchItem, AZ::SystemAllocator);

            MorphTargetDispatchItem() = delete;
            //! Create one dispatch item per morph target
            explicit MorphTargetDispatchItem(
                const AZStd::intrusive_ptr<MorphTargetInputBuffers> inputBuffers,
                const MorphTargetComputeMetaData& morphTargetMetaData,
                SkinnedMeshFeatureProcessor* skinnedMeshFeatureProcessor,
                MorphTargetInstanceMetaData morphInstanceMetaData,
                float accumulatedDeltaRange
            );
            ~MorphTargetDispatchItem();

            // The event handler cannot be copied
            AZ_DISABLE_COPY_MOVE(MorphTargetDispatchItem);

            bool Init();

            const AZ::RHI::DispatchItem& GetRHIDispatchItem() const;

            void SetWeight(float weight);
            float GetWeight() const;
        private:
            bool InitPerInstanceSRG();
            void InitRootConstants(const RHI::ConstantsLayout* rootConstantsLayout);

            // ShaderInstanceNotificationBus::Handler overrides
            void OnShaderReinitialized(const RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const RPI::ShaderVariant& shaderVariant) override;

            RHI::DispatchItem m_dispatchItem;

            // The morph target shader used for this instance
            Data::Instance<RPI::Shader> m_morphTargetShader;

            // The vertex deltas
            AZStd::intrusive_ptr<MorphTargetInputBuffers> m_inputBuffers;

            // The per-object shader resource group
            Data::Instance<RPI::ShaderResourceGroup> m_instanceSrg;

            // Metadata used to set the root constants for the shader
            MorphTargetComputeMetaData m_morphTargetComputeMetaData;

            AZ::RHI::ConstantsData m_rootConstantData;

            // Per-SkinnedMeshInstance constants for morph targets
            MorphTargetInstanceMetaData m_morphInstanceMetaData;
            // A conservative value for encoding/decoding the accumulated deltas
            float m_accumulatedDeltaIntegerEncoding;

            // Keep track of the constant index of m_weight since it is updated frequently
            RHI::ShaderInputConstantIndex m_weightIndex;
        };
    } // namespace Render
} // namespace AZ
