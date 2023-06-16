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
        class ModelAssetHelpers
        {
        public:
            //! Given an empty created ModelAsset, fill it with a valid unit cube model.
            //! This model won't have a material on it so it requires a separate Material component to be displayable.
            //! @param modelAsset An empty modelAsset that will get filled in with unit cube data.
            static void CreateUnitCube(ModelAsset* modelAsset);

        private:
            static Data::Asset<RPI::BufferAsset> CreateBufferAsset(
                const void* data, const uint32_t elementCount, const uint32_t elementSize);
        };
    } //namespace RPI
} // namespace AZ
