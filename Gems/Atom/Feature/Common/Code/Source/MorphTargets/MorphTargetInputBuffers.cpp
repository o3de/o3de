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
            auto buffer = RPI::Buffer::FindOrCreate(bufferAssetView.GetBufferAsset());

            AZ::RHI::Ptr<AZ::RHI::BufferView> bufferView = RHI::Factory::Get().CreateBufferView();
            {
                bufferView->SetName(Name(bufferNamePrefix + "MorphTargetVertexDeltaView"));
                [[maybe_unused]] RHI::ResultCode resultCode = bufferView->Init(*buffer->GetRHIBuffer(), bufferAssetView.GetBufferViewDescriptor());
                AZ_Error("MorphTargetInputBuffers", resultCode == RHI::ResultCode::Success, "Failed to initialize buffer view for morph target.");
            }

            m_vertexDeltaBufferView = bufferView;
        }

        void MorphTargetInputBuffers::SetBufferViewsOnShaderResourceGroup(const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG)
        {
            // Set the delta buffer
            RHI::ShaderInputBufferIndex srgIndex = perInstanceSRG->FindShaderInputBufferIndex(Name{ "m_vertexDeltas" });
            AZ_Error("MorphTargetInputBuffers", srgIndex.IsValid(), "Failed to find shader input index for 'm_positionDeltas' in the skinning compute shader per-instance SRG.");

            [[maybe_unused]] bool success = perInstanceSRG->SetBufferView(srgIndex, m_vertexDeltaBufferView.get());
            AZ_Error("MorphTargetInputBuffers", success, "Failed to bind buffer view for vertex deltas");
        }
    } // namespace Render
}// namespace AZ
