/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace AZ
{
    namespace RPI
    {
        namespace 
        {
            const uint32_t PositionFloatsPerVert = 3;
            const uint32_t NormalFloatsPerVert = 3;
            const uint32_t UVFloatsPerVert = 2;
            const uint32_t ColorFloatsPerVert = 4;
            const uint32_t TangentFloatsPerVert = 4; // The 4th channel is used to indicate handedness of the bitangent, either 1 or -1.
            const uint32_t BitangentFloatsPerVert = 3;

            const AZ::RHI::Format IndicesFormat = AZ::RHI::Format::R32_UINT;
            const AZ::RHI::Format PositionFormat = AZ::RHI::Format::R32G32B32_FLOAT;
            const AZ::RHI::Format NormalFormat = AZ::RHI::Format::R32G32B32_FLOAT;
            const AZ::RHI::Format UVFormat = AZ::RHI::Format::R32G32_FLOAT;
            const AZ::RHI::Format ColorFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;
            const AZ::RHI::Format BitangentFormat = AZ::RHI::Format::R32G32B32_FLOAT;
            // The 4th channel is used to indicate handedness of the bitangent, either 1 or -1.
            const AZ::RHI::Format TangentFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;
            const AZ::RHI::Format SkinIndicesFormat = AZ::RHI::Format::R16_UINT; // Single-component, 16-bit int per weight
            const AZ::RHI::Format SkinWeightFormat = AZ::RHI::Format::R32_FLOAT; // Single-component, 32-bit floating point per weight

            const char* ShaderSemanticName_SkinJointIndices = "SKIN_JOINTINDICES";
            const char* ShaderSemanticName_SkinWeights = "SKIN_WEIGHTS";

            // Morph targets
            const char* ShaderSemanticName_MorphTargetDeltas = "MORPHTARGET_VERTEXDELTAS";

            // Cloth data
            const char* const ShaderSemanticName_ClothData = "CLOTH_DATA";
            const uint32_t ClothDataFloatsPerVert = 4;
            const AZ::RHI::Format ClothDataFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;

            // We align all the skinned mesh related stream buffers to 192 bytes for various reasons.
            // Metal has a restriction where each typed buffer needs to start at 64 byte boundary.
            // At the same time a lot of our stream buffer views are RGB or RGBA so they need to be 12 and
            // 16 byte aligned or all the element count/element offset logic will break. Changing stream buffer
            // views to R32 is also an option but it will break vertex shaders where they expect RGB views for IA buffers (for example t-pose for
            // skinned mesh). In order to satisfy 64/16/12 byte alignment we align all buffers to 192. This way we can meet metal's
            // restriction as well as maintain RGB/RGBA stream buffer views.
            const uint32_t SkinnedMeshBufferAlignment = 192;
        }

        //! ModelAssetHelpers is a collection of helper methods for generating or manipulating model assets.
        class ModelAssetHelpers
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


            template<typename T>
            static size_t GetAlignedCount(size_t vertexCount, AZ::RHI::Format bufferFormat, uint32_t alignment);

            template<typename T>
            static void AlignStreamBuffer(
                AZStd::vector<T>& streamBuffer, size_t vertexCount, AZ::RHI::Format bufferFormat, uint32_t alignment);

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
        size_t ModelAssetHelpers::GetAlignedCount(size_t vertexCount, AZ::RHI::Format bufferFormat, uint32_t alignment)
        {
            //Calculate vertex data in bytes
            size_t vertexDataInBytes = vertexCount * RHI::GetFormatSize(bufferFormat);

            //Align up the appropriate alignment. Alignment can be non-power of two.
            size_t alignedVertexDataInBytes = RHI::AlignUpNPOT(vertexDataInBytes, alignment);

            //Calculate the aligned element count
            size_t alignedVertexCount = alignedVertexDataInBytes / sizeof(T);
            return alignedVertexCount;
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
            size_t alignmentCountDelta = alignedCount - streamBuffer.size();

            // Pad the buffer in order to respect the alignment
            if (alignmentCountDelta > 0)
            {
                AZStd::vector<T> extraIds(alignmentCountDelta, 0);
                streamBuffer.insert(streamBuffer.end(), extraIds.begin(), extraIds.end());
            }
        }
    } //namespace RPI
} // namespace AZ
