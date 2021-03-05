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
        MorphTargetInputBuffers::MorphTargetInputBuffers(const AZStd::vector<uint32_t>& vertexNumbers, const AZStd::vector<uint32_t>& vertexDeltas, const AZStd::string& bufferNamePrefix)
        {
            RHI::BufferViewDescriptor vertexNumberViewDescriptor = RHI::BufferViewDescriptor::CreateTyped(0, vertexNumbers.size(), AZ::RHI::Format::R32_UINT);
            // Use the user-specified buffer name if it exits, or a default one otherwise.
            const char* bufferName = !bufferNamePrefix.empty() ? bufferNamePrefix.c_str() : "MorphTargetVertexNumberBuffer";
            Data::Asset<RPI::BufferAsset> bufferAsset = CreateBufferAsset(vertexNumbers.data(), vertexNumberViewDescriptor, RHI::BufferBindFlags::ShaderRead, SkinnedMeshVertexStreamPropertyInterface::Get()->GetInputStreamResourcePool(), bufferName);
            m_vertexNumbers = RPI::Buffer::FindOrCreate(bufferAsset);
            
            // There are four bytes per element in vertexDeltas
            size_t deltaBufferSizeInBytes = vertexDeltas.size() * 4;
            RHI::BufferViewDescriptor deltaBufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, deltaBufferSizeInBytes);
            // Use the user-specified buffer name if it exits, or a default one otherwise.
            bufferName = !bufferNamePrefix.empty() ? bufferNamePrefix.c_str() : "MorphTargetDeltaBuffer";
            bufferAsset = CreateBufferAsset(vertexDeltas.data(), deltaBufferViewDescriptor, RHI::BufferBindFlags::ShaderRead, SkinnedMeshVertexStreamPropertyInterface::Get()->GetInputStreamResourcePool(), bufferName);
            m_positionDeltas = RPI::Buffer::FindOrCreate(bufferAsset);
        }

        void MorphTargetInputBuffers::SetBufferViewsOnShaderResourceGroup(const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG)
        {
            // Set the vertex numbers
            RHI::ShaderInputBufferIndex srgIndex = perInstanceSRG->FindShaderInputBufferIndex(Name{ "m_vertexIndices" });
            AZ_Error("MorphTargetInputBuffers", srgIndex.IsValid(), "Failed to find shader input index for 'm_vertexNumbers' in the skinning compute shader per-instance SRG.");

            bool success = perInstanceSRG->SetBufferView(srgIndex, m_vertexNumbers->GetBufferView());
            AZ_Error("MorphTargetInputBuffers", success, "Failed to bind buffer view for vertex numbers");

            // Set the position deltas
            srgIndex = perInstanceSRG->FindShaderInputBufferIndex(Name{ "m_vertexDeltas" });
            AZ_Error("MorphTargetInputBuffers", srgIndex.IsValid(), "Failed to find shader input index for 'm_positionDeltas' in the skinning compute shader per-instance SRG.");

            success = perInstanceSRG->SetBufferView(srgIndex, m_positionDeltas->GetBufferView());
            AZ_Error("MorphTargetInputBuffers", success, "Failed to bind buffer view for position deltas");
        }
    } // namespace Render
}// namespace AZ
