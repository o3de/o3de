/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! Use by model assets to identify a logical material slot.
        //! Each slot has a unique ID, a name, and a default material. Each mesh in model will reference a single ModelMaterialSlot.
        //! Other classes like MeshFeatureProcessor and MaterialComponent can override the material associated with individual slots
        //! to alter the default appearance of the mesh.
        struct ModelMaterialSlot
        {
            AZ_TYPE_INFO(ModelMaterialSlot, "{0E88A62A-D83D-4C1B-8DE7-CE972B8124B5}");
                
            static void Reflect(AZ::ReflectContext* context);

            using StableId = uint32_t;
            static const StableId InvalidStableId = -1;

            //! This ID must have a consistent value when the asset is reprocessed by the asset pipeline, and must be unique within the ModelLodAsset.
            //! In practice, this set using the MaterialUid from SceneAPI. See ModelAssetBuilderComponent::CreateMesh.
            StableId m_stableId = InvalidStableId; 

            Name m_displayName; //!< The name of the slot as displayed to the user in UI. (Using Name instead of string for fast copies)

            Data::Asset<MaterialAsset> m_defaultMaterialAsset{ Data::AssetLoadBehavior::PreLoad }; //!< The material that will be applied to this slot by default.
        };

        using ModelMaterialSlotMap = AZStd::unordered_map<ModelMaterialSlot::StableId, ModelMaterialSlot>;

    } //namespace RPI
} // namespace AZ
