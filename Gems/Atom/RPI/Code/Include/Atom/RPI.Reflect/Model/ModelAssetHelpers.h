/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace AZ
{
    namespace RPI
    {
        namespace 
        {
            static constexpr uint32_t PositionFloatsPerVert = 3;
            static constexpr uint32_t NormalFloatsPerVert = 3;
            static constexpr uint32_t UVFloatsPerVert = 2;
            static constexpr uint32_t ColorFloatsPerVert = 4;
            static constexpr uint32_t TangentFloatsPerVert = 4; // The 4th channel is used to indicate handedness of the bitangent, either 1 or -1.
            static constexpr uint32_t BitangentFloatsPerVert = 3;

            static constexpr AZ::RHI::Format IndicesFormat = AZ::RHI::Format::R32_UINT;
            static constexpr AZ::RHI::Format PositionFormat = AZ::RHI::Format::R32G32B32_FLOAT;
            static constexpr AZ::RHI::Format NormalFormat = AZ::RHI::Format::R32G32B32_FLOAT;
            static constexpr AZ::RHI::Format UVFormat = AZ::RHI::Format::R32G32_FLOAT;
            static constexpr AZ::RHI::Format ColorFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;
            static constexpr AZ::RHI::Format BitangentFormat = AZ::RHI::Format::R32G32B32_FLOAT;
            // The 4th channel is used to indicate handedness of the bitangent, either 1 or -1.
            static constexpr AZ::RHI::Format TangentFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;
            static constexpr AZ::RHI::Format SkinIndicesFormat = AZ::RHI::Format::R16_UINT; // Single-component, 16-bit int per weight
            static constexpr AZ::RHI::Format SkinWeightFormat = AZ::RHI::Format::R32_FLOAT; // Single-component, 32-bit floating point per weight

            static constexpr const char* ShaderSemanticName_SkinJointIndices = "SKIN_JOINTINDICES";
            static constexpr const char* ShaderSemanticName_SkinWeights = "SKIN_WEIGHTS";

            // Morph targets
            static constexpr const char* ShaderSemanticName_MorphTargetDeltas = "MORPHTARGET_VERTEXDELTAS";

            // Cloth data
            static constexpr const char* ShaderSemanticName_ClothData = "CLOTH_DATA";
            static constexpr uint32_t ClothDataFloatsPerVert = 4;
            static constexpr AZ::RHI::Format ClothDataFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;

            // We align all the skinned mesh related stream buffers to 192 bytes for various reasons.
            // Metal has a restriction where each typed buffer needs to start at 64 byte boundary.
            // At the same time a lot of our stream buffer views are RGB or RGBA so they need to be 12 and
            // 16 byte aligned or all the element count/element offset logic will break. Changing stream buffer
            // views to R32 is also an option but it will break vertex shaders where they expect RGB views for IA buffers (for example t-pose for
            // skinned mesh). In order to satisfy 64/16/12 byte alignment we align all buffers to 192. This way we can meet metal's
            // restriction as well as maintain RGB/RGBA stream buffer views.
            static constexpr uint32_t SkinnedMeshBufferAlignment = 192;
        }

        //! ModelAssetHelpers is a collection of helper methods for generating or manipulating model assets.
        class ATOM_RPI_REFLECT_API ModelAssetHelpers
        {
        public:
            //! Given an empty created ModelAsset, fill it with a valid unit cube model.
            //! This model won't have a material on it so it requires a separate Material component to be displayable.
            //! @param modelAsset An empty modelAsset that will get filled in with unit cube data.
            static void CreateUnitCube(ModelAsset* modelAsset);

            //! Given an empty created ModelAsset, fill it with a valid unit X-shaped model.
            //! This model won't have a material on it so it requires a separate Material component to be displayable.
            //! @param modelAsset An empty modelAsset that will get filled in with unit X data.
            static void CreateUnitX(ModelAsset* modelAsset);

            //! Given the initial vertex count (vertexCount), format used per vertex (vertexFormat) and the type of each element
            //! in the stream container (T) this function will return an aligned padded value whereby the returnedValue*sizeOf(T) 
            //! is aligned to alignmentInBytes. This will allow the higher level user to pad their container holding vertex data so that it
            //! ends at alignmentInBytes byte boundary. For example if you have a buffer with 2 vertices using format RGB32 stored in
            //! a float container and we want to align it to 36 byte boundary this method will return 9 elements as 9 elements end at
            //! 36 byte boundary and at the same time 9 means padding of exactly one extra vertex as each vertex needs 3 floats.
            //! In the scenario above the function assumes that 36 will be a multiple of 12 (size of RGB32) as well as 4(size of float). 
            //! 
            //! @param vertexCount Number of vertices that need to be aligned
            //! @param vertexFormat Format associated with each vertex
            //! @param alignmentInBytes Alignment value in bytes
            //! @param T The type associated with container holding vertex data. For example it could be float, uint32_t or uin16_t.
            template<typename T>
            static size_t GetAlignedCount(size_t vertexCount, AZ::RHI::Format vertexFormat, uint32_t alignmentInBytes);

            //! This function will pad the vertex stream buffer (vertexStreamBuffer) with 0s in order to ensure that the total size
            //! of the buffer ends at the alignmentInBytes byte boundary.
            //! @param vertexStreamBuffer Vertex stream buffer that require padding
            //! @param vertexCount Number of vertices that need to be aligned
            //! @param vertexFormat Format associated with each vertex
            //! @param alignmentInBytes Alignment value in bytes
            //! @param T The type associated with container holding vertex stream data. For example it could be float, uint32_t or uin16_t.
            template<typename T>
            static void AlignStreamBuffer(
                AZStd::vector<T>& vertexStreamBuffer, size_t vertexCount, AZ::RHI::Format vertexFormat, uint32_t alignmentInBytes);

        private:
            //! Create a BufferAsset from the given data buffer.
            //! @param data The data buffer to use for the BufferAsset
            //! @param elementCount The number of elements in the data buffer
            //! @param elementSize The size of each element in the data buffer in bytes
            static Data::Asset<RPI::BufferAsset> CreateBufferAsset(
                const void* data, const uint32_t elementCount, const uint32_t elementSize);

            //! Create a model from the given data buffers.
            //! @param modelAsset An empty modelAsset that will get filled in with the model data.
            //! @param name The name to use for the model
            //! @param indices The index buffer
            //! @param positions The position buffer
            //! @param normals The normal buffer
            //! @param tangents The tangent buffer
            //! @param bitangents The bitangent buffer
            //! @param uvs The UV buffer
            static void CreateModel(
                ModelAsset* modelAsset,
                const AZ::Name& name,
                AZStd::span<const uint32_t> indices,
                AZStd::span<const float_t> positions,
                AZStd::span<const float> normals,
                AZStd::span<const float> tangents,
                AZStd::span<const float> bitangents,
                AZStd::span<const float> uvs);
        };

        template<typename T>
        size_t ModelAssetHelpers::GetAlignedCount(size_t vertexCount, AZ::RHI::Format vertexFormat, uint32_t alignmentInBytes)
        {
            //Size in bytes used by one vertex
            uint32_t vertexFormatSizeInBytes = RHI::GetFormatSize(vertexFormat);

            //Size in bytes used by an element within the container holding vertex stream buffer.
            uint32_t vertexContainerElementSizeInBytes = sizeof(T);

            AZ_Assert(alignmentInBytes % vertexFormatSizeInBytes == 0, "alignmentInBytes needs to be a multiple of bytes used per vertex");
            AZ_Assert(alignmentInBytes % vertexContainerElementSizeInBytes == 0,
                "alignmentInBytes needs to be a multiple of bytes used by an element within the vertex stream buffer container");

            //Calculate vertex data in bytes
            size_t vertexDataInBytes = vertexCount * vertexFormatSizeInBytes;

            //Align up the appropriate alignment. Alignment can be non-power of two.
            size_t alignedVertexDataInBytes = RHI::AlignUpNPOT(vertexDataInBytes, alignmentInBytes);

            //Calculate the aligned element count
            size_t alignedCount = alignedVertexDataInBytes / vertexContainerElementSizeInBytes;
            return alignedCount;
        }

        template<typename T>
        void ModelAssetHelpers::AlignStreamBuffer(
            AZStd::vector<T>& streamBuffer, size_t vertexCount, AZ::RHI::Format bufferFormat, uint32_t alignment)
        {
            size_t alignedCount = GetAlignedCount<T>(vertexCount, bufferFormat, alignment);
            AZ_Assert(
                alignedCount >= streamBuffer.size(),
                "Aligned count should be equal or greater as we are aligning up. Aligned value %i BufferSize %i",
                alignedCount,
                streamBuffer.size());
            streamBuffer.resize(alignedCount, static_cast<T>(0));
        }
    } //namespace RPI
} // namespace AZ
