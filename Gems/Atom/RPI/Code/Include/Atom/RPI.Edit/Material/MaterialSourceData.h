/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/JSON/document.h>

#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! A reserved name used in material inspectors.
        //! In the source data, properties and UV names are loaded separately.
        //! However, treating UV names as a special property group can greatly simplify the editor code.
        //! See MaterialInspector::AddUvNamesGroup() for more details.
        static constexpr const char UvGroupName[] = "uvSets";

        class MaterialAsset;

        //! This is a simple data structure for serializing in/out material source files.
        class MaterialSourceData final
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::MaterialSourceData, "{B8881D92-DF9F-4552-9F22-FF4421C45D9A}");

            static constexpr const char Extension[] = "material";

            static void Reflect(ReflectContext* context);

            MaterialSourceData() = default;
            
            AZStd::string m_description;
            
            AZStd::string m_materialType; //!< The material type that defines the interface and behavior of the material
            
            AZStd::string m_parentMaterial; //!< The immediate parent of this material

            uint32_t m_materialTypeVersion = 0; //!< The version of the material type that was used to configure this material

            struct Property
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialSourceData::Property, "{8D613464-3750-4122-AFFE-9238010D5AFC}");

                MaterialPropertyValue m_value;
            };

            using PropertyMap = AZStd::map<AZStd::string, Property>;
            using PropertyGroupMap = AZStd::map<AZStd::string, PropertyMap>;

            PropertyGroupMap m_properties;

            enum class ApplyVersionUpdatesResult
            {
                Failed,
                NoUpdates,
                UpdatesApplied
            };

            //! Checks the material type version and potentially applies a series of property changes (most common are simple property renames)
            //! based on the MaterialTypeAsset's version update procedure.
            //! @param materialSourceFilePath Indicates the path of the .material file that the MaterialSourceData represents. Used for resolving file-relative paths.
            ApplyVersionUpdatesResult ApplyVersionUpdates(AZStd::string_view materialSourceFilePath = "");

            //! Creates a MaterialAsset from the MaterialSourceData content.
            //! @param assetId ID for the MaterialAsset
            //! @param materialSourceFilePath Indicates the path of the .material file that the MaterialSourceData represents. Used for resolving file-relative paths.
            //! @param elevateWarnings Indicates whether to treat warnings as errors
            //! @param includeMaterialPropertyNames Indicates whether to save material property names into the material asset file
            Outcome<Data::Asset<MaterialAsset>> CreateMaterialAsset(
                Data::AssetId assetId,
                AZStd::string_view materialSourceFilePath = "",
                bool elevateWarnings = true,
                bool includeMaterialPropertyNames = true
            ) const;
        };
    } // namespace RPI
} // namespace AZ
