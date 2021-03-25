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
        MorphTargetInputBuffers::MorphTargetInputBuffers(uint32_t vertexCount, const AZStd::vector<uint32_t>& vertexDeltas, const AZStd::string& bufferNamePrefix)
        {            
            // There are four bytes per element in vertexDeltas
            size_t deltaBufferSizeInBytes = vertexDeltas.size() * sizeof(uint32_t);
            size_t deltaElementSizeInBytes = deltaBufferSizeInBytes / vertexCount;
            AZ_Assert(deltaElementSizeInBytes % 16 == 0, "MorphTargetInputBuffers - Morph target deltas must be 16 byte aligned for structured buffers.");
            RHI::BufferViewDescriptor deltaBufferViewDescriptor = RHI::BufferViewDescriptor::CreateStructured(0, vertexCount, deltaElementSizeInBytes);

            // Use the user-specified buffer name if it exits, or a default one otherwise.
            const char* bufferName = !bufferNamePrefix.empty() ? bufferNamePrefix.c_str() : "MorphTargetDeltaBuffer";

            // Create the actual buffer
            Data::Asset<RPI::BufferAsset> bufferAsset = CreateBufferAsset(vertexDeltas.data(), deltaBufferViewDescriptor, RHI::BufferBindFlags::ShaderRead, SkinnedMeshVertexStreamPropertyInterface::Get()->GetInputStreamResourcePool(), bufferName);
            m_vertexDeltas = RPI::Buffer::FindOrCreate(bufferAsset);
        }

        void MorphTargetInputBuffers::SetBufferViewsOnShaderResourceGroup(const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG)
        {
            // Set the delta buffer
            RHI::ShaderInputBufferIndex srgIndex = perInstanceSRG->FindShaderInputBufferIndex(Name{ "m_vertexDeltas" });
            AZ_Error("MorphTargetInputBuffers", srgIndex.IsValid(), "Failed to find shader input index for 'm_positionDeltas' in the skinning compute shader per-instance SRG.");

            bool success = perInstanceSRG->SetBufferView(srgIndex, m_vertexDeltas->GetBufferView());
            AZ_Error("MorphTargetInputBuffers", success, "Failed to bind buffer view for vertex deltas");
        }
    } // namespace Render
}// namespace AZ
