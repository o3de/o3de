/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>
#include <SkinnedMesh/SkinnedMeshOutputStreamManager.h>

#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RHI/Factory.h>

#include <AzCore/std/algorithm.h>
#include <AzCore/Math/PackedVector3.h>

namespace AZ
{
    namespace Render
    {
        MorphTargetInputBuffers::MorphTargetInputBuffers(const RPI::BufferAssetView& bufferAssetView, const AZStd::string& bufferNamePrefix)
        {            
            m_vertexDeltaBuffer = RPI::Buffer::FindOrCreate(bufferAssetView.GetBufferAsset());
            if (m_vertexDeltaBuffer)
            {
                m_vertexDeltaBufferView = m_vertexDeltaBuffer->GetRHIBuffer()->BuildBufferView(bufferAssetView.GetBufferViewDescriptor());
                m_vertexDeltaBufferView->SetName(Name(bufferNamePrefix + "MorphTargetVertexDeltaView"));
            }
        }

        void MorphTargetInputBuffers::SetBufferViewsOnShaderResourceGroup(const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG)
        {
            // Set the delta buffer
            RHI::ShaderInputBufferIndex srgIndex = perInstanceSRG->FindShaderInputBufferIndex(Name{ "m_vertexDeltas" });
            AZ_Error("MorphTargetInputBuffers", srgIndex.IsValid(), "Failed to find shader input index for 'm_positionDeltas' in the skinning compute shader per-instance SRG.");

            [[maybe_unused]] bool success = perInstanceSRG->SetBufferView(
                srgIndex, m_vertexDeltaBufferView.get());
            AZ_Error("MorphTargetInputBuffers", success, "Failed to bind buffer view for vertex deltas");
        }
    } // namespace Render
}// namespace AZ
