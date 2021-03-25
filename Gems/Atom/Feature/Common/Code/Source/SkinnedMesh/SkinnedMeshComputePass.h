/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <SkinnedMesh/SkinnedMeshShaderOptionsCache.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    namespace Render
    {
        //! The skinned mesh compute pass submits dispatch items for skinning. The dispatch items are cleared every frame, so it needs to be re-populated.
        class SkinnedMeshComputePass
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(SkinnedMeshComputePass);
        public:
            AZ_RTTI(AZ::Render::SkinnedMeshComputePass, "{CE046FFC-B870-40EE-872A-DB0958B97CC3}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(SkinnedMeshComputePass, SystemAllocator, 0);

            SkinnedMeshComputePass(const RPI::PassDescriptor& descriptor);

            static RPI::Ptr<SkinnedMeshComputePass> Create(const RPI::PassDescriptor& descriptor);

            //! Thread-safe function for adding a dispatch item to the current frame.
            void AddDispatchItem(const RHI::DispatchItem* dispatchItem);
            Data::Instance<RPI::Shader> GetShader() const;
            RPI::ShaderOptionGroup CreateShaderOptionGroup(const SkinnedMeshShaderOptions shaderOptions, SkinnedMeshShaderOptionNotificationBus::Handler& shaderReinitializedHandler);

        private:
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const RPI::Shader& shader) override;
            void OnShaderVariantReinitialized(const RPI::Shader& shader, const RPI::ShaderVariantId& shaderVariantId, RPI::ShaderVariantStableId shaderVariantStableId) override;

            AZStd::mutex m_mutex;
            AZStd::unordered_set<const RHI::DispatchItem*> m_dispatches;
            CachedSkinnedMeshShaderOptions m_cachedShaderOptions;
        };
    }
}
