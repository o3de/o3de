/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/map.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class MaterialTypeAsset;
        class MaterialFunctorSourceDataHolder;

        //! This is a simple data structure for serializing in/out material type source files.
        class MaterialTypeSourceData final
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData, "{14085B6F-42E8-447D-9833-E1E45C2510B2}");
            AZ_CLASS_ALLOCATOR(MaterialTypeSourceData, SystemAllocator, 0);

            static constexpr const char Extension[] = "materialtype";

            static void Reflect(ReflectContext* context);

            struct PropertyConnection
            {
            public:
                AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData::PropertyConnection, "{C2F37C26-D7EF-4142-A650-EF50BB18610F}");

                PropertyConnection() = default;
                PropertyConnection(MaterialPropertyOutputType type, AZStd::string_view fieldName, int32_t shaderIndex = -1);

                MaterialPropertyOutputType m_type = MaterialPropertyOutputType::Invalid;

                //! The name of a specific shader setting. This will either be a ShaderResourceGroup input or a ShaderOption, depending on m_type
                AZStd::string m_fieldName;

                //! For m_type==ShaderOption, this is either the index of a specific shader in m_shaderCollection, or -1 which means every shader in m_shaderCollection.
                //! For m_type==ShaderInput, this field is not used.
                int32_t m_shaderIndex = -1; 
            };

            using PropertyConnectionList = AZStd::vector<PropertyConnection>;

            struct GroupDefinition
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData::GroupDefinition, "{B2D0FC5C-72A3-435E-A194-1BFDABAC253D}");

                //! The unique name of the property group. The full property ID will be groupName.propertyName
                AZStd::string m_name;

                // Editor metadata ...
                AZStd::string m_displayName;
                AZStd::string m_description;
            };

            struct PropertyDefinition
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData::PropertyDefinition, "{E0DB3C0D-75DB-4ADB-9E79-30DA63FA18B7}");

                static const float DefaultMin;
                static const float DefaultMax;
                static const float DefaultStep;

                AZStd::string m_name; //!< The name of the property within the property group. The full property ID will be groupName.propertyName.

                MaterialPropertyVisibility m_visibility = MaterialPropertyVisibility::Default;

                MaterialPropertyDataType m_dataType = MaterialPropertyDataType::Invalid;

                PropertyConnectionList m_outputConnections; //!< List of connections from material property to shader settings

                MaterialPropertyValue m_value; //!< Value for the property. The type must match the MaterialPropertyDataType.

                AZStd::vector<AZStd::string> m_enumValues; //!< Only used if property is Enum type
                bool m_enumIsUv = false;  //!< Indicates if the enum value should use m_enumValues or those extracted from m_uvNameMap.

                // Editor metadata ...
                AZStd::string m_displayName;
                AZStd::string m_description;
                AZStd::vector<AZStd::string> m_vectorLabels;
                MaterialPropertyValue m_min;
                MaterialPropertyValue m_max;
                MaterialPropertyValue m_softMin;
                MaterialPropertyValue m_softMax;
                MaterialPropertyValue m_step;
            };

            struct ShaderVariantReferenceData
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData::ShaderVariantReferenceData, "{927F3AAE-C0A9-4B79-B773-A97211E4E514}");

                ShaderVariantReferenceData() = default;
                explicit ShaderVariantReferenceData(AZStd::string_view shaderFilePath) : m_shaderFilePath(shaderFilePath) {}
                
                //! Path to a ".shader" file, relative to the asset root
                AZStd::string m_shaderFilePath;

                //! Unique tag to identify the shader
                AZ::Name m_shaderTag;

                //! This list provides a way for users to set shader option values in a 'hard-coded' way rather than connecting them to material properties.
                //! These are optional and the list will usually be empty; most options will either get set from a material property connection,
                //! or will use the default value from the shader. 
                AZStd::unordered_map<Name/*shaderOption*/, Name/*value*/> m_shaderOptionValues;
            };

            using PropertyList = AZStd::vector<PropertyDefinition>;

            struct VersionUpdatesRenameOperationDefinition
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData::VersionUpdatesRenameOperationDefinition, "{F2295489-E15A-46CC-929F-8D42DEDBCF14}");

                AZStd::string m_operation;
                
                AZStd::string m_renameFrom;
                AZStd::string m_renameTo;
            };

            // TODO: Support script operations--At that point, we'll likely need to replace VersionUpdatesRenameOperationDefinition with a more generic
            // data structure that has a custom JSON serialize. We will only be supporting rename for now.
            using VersionUpdateActions = AZStd::vector<VersionUpdatesRenameOperationDefinition>;

            struct VersionUpdateDefinition
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData::VersionUpdateDefinition, "{2C9D3B91-0585-4BC9-91D2-4CF0C71BC4B7}");

                uint32_t m_toVersion;
                VersionUpdateActions m_actions;
            };

            using VersionUpdates = AZStd::vector<VersionUpdateDefinition>;

            struct PropertyLayout
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData::PropertyLayout, "{AE53CF3F-5C3B-44F5-B2FB-306F0EB06393}");
                
                //! This field is unused, and has been replaced by MaterialTypeSourceData::m_version below. It is kept for legacy file compatibility to suppress warnings and errors.
                uint32_t m_versionOld = 0;

                //! List of groups that will contain the available properties
                AZStd::vector<GroupDefinition> m_groups;

                //! Collection of all available user-facing properties
                AZStd::map<AZStd::string /*group name*/, PropertyList> m_properties;
            };

            AZStd::string m_description;

            //! Version 1 is the default and should not contain any version update.
            uint32_t m_version = 1;

            VersionUpdates m_versionUpdates;

            PropertyLayout m_propertyLayout;

            //! A list of shader variants that are always used at runtime; they cannot be turned off
            AZStd::vector<ShaderVariantReferenceData> m_shaderCollection;

            //! Material functors provide custom logic and calculations to configure shaders, render states, and more. See MaterialFunctor.h for details.
            AZStd::vector<Ptr<MaterialFunctorSourceDataHolder>> m_materialFunctorSourceData;

            //! Override names for UV input in the shaders of this material type.
            //! Using ordered map to sort names on loading.
            using UvNameMap = AZStd::map<AZStd::string, AZStd::string>;
            UvNameMap m_uvNameMap;

            //! Copy over UV custom names to the properties enum values.
            void ResolveUvEnums();

            const GroupDefinition* FindGroup(AZStd::string_view groupName) const;

            //! Searches for a specific property. 
            //! Note this function can find properties using old versions of the property name; in that case,
            //! the name in the returned PropertyDefinition* will not match the @propertyName that was searched for.
            //! @param materialTypeVersion indicates the version number of the property name being passed in. Only renames above this version number will be applied.
            //! @return the requested property, or null if it could not be found
            const PropertyDefinition* FindProperty(AZStd::string_view groupName, AZStd::string_view propertyName, uint32_t materialTypeVersion = 0) const;

            //! Construct a complete list of group definitions, including implicit groups, arranged in the same order as the source data
            //! Groups with the same name will be consolidated into a single entry
            AZStd::vector<GroupDefinition> GetGroupDefinitionsInDisplayOrder() const;

            //! Call back function type used with the numeration functions
            using EnumeratePropertiesCallback = AZStd::function<bool(
                const AZStd::string&, // The name of the group containing the property
                const AZStd::string&, // The name of the property
                const PropertyDefinition& // the property definition object that corresponds to the group and property names
                )>;

            //! Traverse all of the properties contained in the source data executing a callback function
            //! Traversal will occur in group alphabetical order and stop once all properties have been enumerated or the callback function returns false
            void EnumerateProperties(const EnumeratePropertiesCallback& callback) const;

            //! Traverse all of the properties in the source data in display/storage order executing a callback function
            //! Traversal will stop once all properties have been enumerated or the callback function returns false
            void EnumeratePropertiesInDisplayOrder(const EnumeratePropertiesCallback& callback) const;

            //! Convert the property value into the format that will be stored in the source data
            //! This is primarily needed to support conversions of special types like enums and images
            bool ConvertPropertyValueToSourceDataFormat(const PropertyDefinition& propertyDefinition, MaterialPropertyValue& propertyValue) const;

            Outcome<Data::Asset<MaterialTypeAsset>> CreateMaterialTypeAsset(Data::AssetId assetId, AZStd::string_view materialTypeSourceFilePath = "", bool elevateWarnings = true) const;
            
            //! Possibly renames @propertyId based on the material version update steps.
            //! @param materialTypeVersion indicates the version number of the property name being passed in. Only renames above this version number will be applied.
            //! @return true if the property was renamed
            bool ApplyPropertyRenames(MaterialPropertyId& propertyId, uint32_t materialTypeVersion = 0) const;
        };

        //! The wrapper class for derived material functors.
        //! It is used in deserialization so that derived material functors can be deserialized by name.
        class MaterialFunctorSourceDataHolder final
            : public AZStd::intrusive_base
        {
            friend class JsonMaterialFunctorSourceDataSerializer;

        public:
            AZ_RTTI(MaterialFunctorSourceDataHolder, "{073C98F6-9EA4-411A-A6D2-A47428A0EFD4}");
            AZ_CLASS_ALLOCATOR(MaterialFunctorSourceDataHolder, AZ::SystemAllocator, 0);

            MaterialFunctorSourceDataHolder() = default;
            MaterialFunctorSourceDataHolder(Ptr<MaterialFunctorSourceData> actualSourceData);
            ~MaterialFunctorSourceDataHolder() = default;

            static void Reflect(AZ::ReflectContext* context);

            MaterialFunctorSourceData::FunctorResult CreateFunctor(const MaterialFunctorSourceData::RuntimeContext& runtimeContext) const
            {
                return m_actualSourceData ? m_actualSourceData->CreateFunctor(runtimeContext) : Failure();
            }
            MaterialFunctorSourceData::FunctorResult CreateFunctor(const MaterialFunctorSourceData::EditorContext& editorContext) const
            {
                return m_actualSourceData ? m_actualSourceData->CreateFunctor(editorContext) : Failure();
            }

            const Ptr<MaterialFunctorSourceData> GetActualSourceData() const { return m_actualSourceData; }
        private:
            Ptr<MaterialFunctorSourceData> m_actualSourceData = nullptr; // The derived material functor instance.
        };
    } // namespace RPI

} // namespace AZ
