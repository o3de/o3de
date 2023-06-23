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
    } //namespace RPI
} // namespace AZ
