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
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
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
        class MaterialAssetCreator;

        enum class MaterialAssetProcessingMode
        {
            PreBake,      //!< all material asset processing is done in the Asset Processor, producing a finalized material asset
            DeferredBake  //!< some material asset processing is deferred, and the material asset is finalized at runtime after loading
        };

        //! This is a simple data structure for serializing in/out material source files.
        class MaterialSourceData final
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::MaterialSourceData, "{B8881D92-DF9F-4552-9F22-FF4421C45D9A}");

            static constexpr const char Extension[] = "material";

            static void Reflect(ReflectContext* context);
            
            //! Creates a MaterialSourceData object that includes the default values for every possible property in the material type.
            static MaterialSourceData CreateAllPropertyDefaultsMaterial(const Data::Asset<MaterialTypeAsset>& materialType, const AZStd::string& materialTypeSourcePath);

            MaterialSourceData() = default;
            
            AZStd::string m_description;
            
            AZStd::string m_materialType; //!< The material type that defines the interface and behavior of the material
            
            AZStd::string m_parentMaterial; //!< The immediate parent of this material

            uint32_t m_materialTypeVersion = MaterialAsset::UnspecifiedMaterialTypeVersion; //!< The version of the material type that was used to configure this material

            enum class ApplyVersionUpdatesResult
            {
                Failed,
                NoUpdates,
                UpdatesApplied
            };
            
            //! If the data was loaded from an old format file (i.e. where "properties" was a tree with property values nested under groups),
            //! this converts to the new format where properties are stored in a flat list.
            void UpgradeLegacyFormat();

            // Note that even though we use an unordered map, the JSON serialization system is nice enough to sort the data when saving to JSON.
            using PropertyValueMap = AZStd::unordered_map<Name, MaterialPropertyValue>;

            void SetPropertyValue(const Name& propertyId, const MaterialPropertyValue& value);
            const MaterialPropertyValue& GetPropertyValue(const Name& propertyId) const;
            const PropertyValueMap& GetPropertyValues() const;
            bool HasPropertyValue(const Name& propertyId) const;
            void RemovePropertyValue(const Name& propertyId);

            //! Creates a MaterialAsset from the MaterialSourceData content.
            //! @param assetId ID for the MaterialAsset
            //! @param materialSourceFilePath Indicates the path of the .material file that the MaterialSourceData represents. Used for
            //! resolving file-relative paths.
            //! @param processingMode Indicates whether to finalize the material asset using data from the MaterialTypeAsset.
            //! @param elevateWarnings Indicates whether to treat warnings as errors
            Outcome<Data::Asset<MaterialAsset>> CreateMaterialAsset(
                Data::AssetId assetId,
                const AZStd::string& materialSourceFilePath,
                MaterialAssetProcessingMode processingMode,
                bool elevateWarnings = true) const;

            //! Creates a MaterialAsset from the MaterialSourceData content.
            //! @param assetId ID for the MaterialAsset
            //! @param materialSourceFilePath Indicates the path of the .material file that the MaterialSourceData represents. Used for
            //! resolving file-relative paths.
            //! @param elevateWarnings Indicates whether to treat warnings as errors
            //! @param sourceDependencies if not null, will be populated with a set of all of the loaded material and material type paths
            Outcome<Data::Asset<MaterialAsset>> CreateMaterialAssetFromSourceData(
                Data::AssetId assetId,
                AZStd::string_view materialSourceFilePath = "",
                bool elevateWarnings = true,
                AZStd::unordered_set<AZStd::string>* sourceDependencies = nullptr) const;

        private:

            void ApplyPropertiesToAssetCreator(
                AZ::RPI::MaterialAssetCreator& materialAssetCreator, const AZStd::string_view& materialSourceFilePath) const;

            // @deprecated: Don't use "properties" in JSON, use "propertyValues" instead.
            using PropertyGroupMap = AZStd::unordered_map<Name, PropertyValueMap>;
            PropertyGroupMap m_propertiesOld;

            PropertyValueMap m_propertyValues;
            MaterialPropertyValue m_invalidValue;
        };
    } // namespace RPI
} // namespace AZ
