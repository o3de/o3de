/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshRenderProxyInterface.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshInstance.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshShaderOptions.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace Render
    {
        class SkinnedMeshInputBuffers;

        //! SkinnedMeshFeatureProcessorInterface provides an interface to acquire and release a SkinnedMeshRenderProxy from the underlying SkinnedMeshFeatureProcessor
        class SkinnedMeshFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::SkinnedMeshFeatureProcessorInterface, "{6BE6D9D7-FFD7-4C35-9A84-4EFDE730F06B}");

            struct SkinnedMeshRenderProxyDesc
            {
                Data::Instance<SkinnedMeshInputBuffers> m_inputBuffers;
                AZStd::intrusive_ptr<SkinnedMeshInstance> m_instance;
                AZStd::shared_ptr<MeshFeatureProcessorInterface::MeshHandle> m_meshHandle;
                Data::Instance<RPI::Buffer> m_boneTransforms;
                SkinnedMeshShaderOptions m_shaderOptions;
            };
            virtual SkinnedMeshRenderProxyInterfaceHandle AcquireRenderProxyInterface(const SkinnedMeshRenderProxyDesc& desc) = 0;
            virtual bool ReleaseRenderProxyInterface(SkinnedMeshRenderProxyInterfaceHandle& handle) = 0;
        };
    } // namespace Render
} // namespace AZ
