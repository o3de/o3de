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
#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Shader/ShaderOptionValuesSourceData.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class MaterialTypeAsset;
        class MaterialTypeAssetCreator;
        class MaterialFunctorSourceDataHolder;
        class JsonMaterialPropertySerializer;

        //! This is a simple data structure for serializing in/out .materialtype source files.
        //! The .materialtype file has two slightly different formats: "abstract" and "direct". See enum Format below.
        class MaterialTypeSourceData final
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData, "{14085B6F-42E8-447D-9833-E1E45C2510B2}");
            AZ_CLASS_ALLOCATOR(MaterialTypeSourceData, SystemAllocator, 0);

            static constexpr const char Extension[] = "materialtype";

            static void Reflect(ReflectContext* context);

            //! The .materialtype file has two slightly different formats, in most cases users will want to author content in the abstract format, which is 
            //! more convenient to work with, as it hides of lot of technical details and automatically works with mulitple render pipelines.
            enum class Format
            {
                Invalid,

                //! In the abstract format, the material type provides only material-specific shader code and a lighting model reference.
                //! The MaterialTypeBuilder will automatically adapt the material type to work in any render pipeline (Forward+, Deferred, VR, etc.),
                //! by stitching it together with the available material pipelines (see MaterialPipelineSourceData). This will produce a
                //! new intermediate material type that is not abstract, for further processing. 
                Abstract,

                //! In the direct format, the material type provides a complete list of the specific shaders that will be used for rendering.
                //! This circumvents the material pipeline system, and the author is responsible for adapting the material type to any desired render pipelines.
                Direct
            };

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
                friend class JsonMaterialPropertySerializer;

                AZ_CLASS_ALLOCATOR(PropertyDefinition, SystemAllocator, 0);
                AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData::PropertyDefinition, "{E0DB3C0D-75DB-4ADB-9E79-30DA63FA18B7}");
                
                static const float DefaultMin;
                static const float DefaultMax;
                static const float DefaultStep;
                
                PropertyDefinition() = default;

                explicit PropertyDefinition(AZStd::string_view name) : m_name(name)
                {
                }

                const AZStd::string& GetName() const { return m_name; }

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

            private:

                // We are gradually moving toward having a more proper API for MaterialTypeSourceData code, but we still some public members
                // like above. However, it's important for m_name to be private because it is used as the key for lookups, collision validation, etc.
                AZStd::string m_name; //!< The name of the property within the property group. The full property ID will be groupName.propertyName.
            };
            
            using PropertyList = AZStd::vector<AZStd::unique_ptr<PropertyDefinition>>;

            struct PropertyGroup
            {
                friend class MaterialTypeSourceData;
                
                AZ_CLASS_ALLOCATOR(PropertyGroup, SystemAllocator, 0);
                AZ_TYPE_INFO(AZ::RPI::MaterialTypeSourceData::PropertyGroup, "{BA3AA0E4-C74D-4FD0-ADB2-00B060F06314}");

            public:

                PropertyGroup() = default;
                AZ_DISABLE_COPY(PropertyGroup)

                const AZStd::string& GetName() const;
                const AZStd::string& GetDisplayName() const;
                const AZStd::string& GetDescription() const;
                const PropertyList& GetProperties() const;
                const AZStd::string& GetShaderInputsPrefix() const;
                const AZStd::string& GetShaderOptionsPrefix() const;
                const AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& GetPropertyGroups() const;
                const AZStd::vector<Ptr<MaterialFunctorSourceDataHolder>>& GetFunctors() const;
                
                void SetDisplayName(AZStd::string_view displayName);
                void SetDescription(AZStd::string_view description);

                //! Add a new property to this PropertyGroup.
                //! @param name a unique for the property. Must be a C-style identifier.
                //! @return the new PropertyDefinition, or null if the name was not valid.
                PropertyDefinition* AddProperty(AZStd::string_view name);
                
                //! Add a new nested PropertyGroup to this PropertyGroup.
                //! @param name a unique for the property group. Must be a C-style identifier.
                //! @return the new PropertyGroup, or null if the name was not valid.
                PropertyGroup* AddPropertyGroup(AZStd::string_view name);
                
            private:

                static PropertyGroup* AddPropertyGroup(AZStd::string_view name, AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& toPropertyGroupList);

                AZStd::string m_name;
                AZStd::string m_displayName;
                AZStd::string m_description;
                AZStd::string m_shaderInputsPrefix;  //!< The name of all SRG inputs under this group will get this prefix.
                AZStd::string m_shaderOptionsPrefix; //!< The name of all shader options under this group will get this prefix.
                PropertyList m_properties;
                AZStd::vector<AZStd::unique_ptr<PropertyGroup>> m_propertyGroups;
                AZStd::vector<Ptr<MaterialFunctorSourceDataHolder>> m_materialFunctorSourceData;
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
                ShaderOptionValuesSourceData m_shaderOptionValues;
            };

            using VersionUpdateActions = AZStd::vector<MaterialVersionUpdate::Action::ActionDefinition>;

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

                PropertyLayout() = default;
                AZ_DISABLE_COPY(PropertyLayout)

                //! This field is unused, and has been replaced by MaterialTypeSourceData::m_version below. It is kept for legacy file compatibility to suppress warnings and errors.
                uint32_t m_versionOld = 0;

                //! @deprecated: Use m_propertyGroups instead
                //! List of groups that will contain the available properties
                AZStd::vector<GroupDefinition> m_groupsOld;

                //! @deprecated: Use m_propertyGroups instead
                AZStd::map<AZStd::string /*group name*/, AZStd::vector<PropertyDefinition>> m_propertiesOld;
                
                //! Collection of all available user-facing properties
                AZStd::vector<AZStd::unique_ptr<PropertyGroup>> m_propertyGroups;
            };
            
            AZStd::string m_description;

            //! Version 1 is the default and should not contain any version update.
            uint32_t m_version = 1;
            
            VersionUpdates m_versionUpdates;

            //! A list of specific shaders that will be used to render the material.
            AZStd::vector<ShaderVariantReferenceData> m_shaderCollection;

            //! This indicates the name of the lighting model that this material type uses.
            //! For example, "Standard", "Enhanced", "Skin". The actual set of available lighting models
            //! is determined by the .materialpipeline.
            //! This is relevant for "abstract" material type files (see IsAbstractFormat()).
            AZStd::string m_lightingModel;

            //! This indicates a .azsli file that contains only material-specific shader code.
            //! The build system will automatically combine this code with .materialpipeline shader code
            //! for use in each available render pipeline.
            //! This is relevant for "abstract" material type files (see IsAbstractFormat()).
            AZStd::string m_materialShaderCode;

            //! Material functors provide custom logic and calculations to configure shaders, render states, and more. See MaterialFunctor.h for details.
            AZStd::vector<Ptr<MaterialFunctorSourceDataHolder>> m_materialFunctorSourceData;

            //! Override names for UV input in the shaders of this material type.
            //! Using ordered map to sort names on loading.
            using UvNameMap = AZStd::map<AZStd::string, AZStd::string>;
            UvNameMap m_uvNameMap;

            //! Copy over UV custom names to the properties enum values.
            void ResolveUvEnums();

            //! Add a new PropertyGroup for containing properties or other PropertyGroups.
            //! @param propertyGroupId The ID of the new property group. To add as a nested PropertyGroup, use a full path ID like "levelA.levelB.levelC"; in this case a property group "levelA.levelB" must already exist.
            //! @return a pointer to the new PropertyGroup or null if there was a problem (an AZ_Error will be reported).
            PropertyGroup* AddPropertyGroup(AZStd::string_view propertyGroupId);

            //! Add a new property to a PropertyGroup.
            //! @param propertyId The ID of the new property, like "layerBlend.factor" or "layer2.roughness.texture". The indicated property group must already exist.
            //! @return a pointer to the new PropertyDefinition or null if there was a problem (an AZ_Error will be reported).
            PropertyDefinition* AddProperty(AZStd::string_view propertyId);

            //! Return the PropertyLayout containing the tree of property groups and property definitions.
            const PropertyLayout& GetPropertyLayout() const { return m_propertyLayout; }

            //! Find the PropertyGroup with the given ID.
            //! @param propertyGroupId The full ID of a property group to find, like "levelA.levelB.levelC".
            //! @return the found PropertyGroup or null if it doesn't exist.
            const PropertyGroup* FindPropertyGroup(AZStd::string_view propertyGroupId) const;
            PropertyGroup* FindPropertyGroup(AZStd::string_view propertyGroupId);
            
            //! Find the definition for a property with the given ID.
            //! @param propertyId The full ID of a property to find, like "baseColor.texture".
            //! @return the found PropertyDefinition or null if it doesn't exist.
            const PropertyDefinition* FindProperty(AZStd::string_view propertyId) const;
            PropertyDefinition* FindProperty(AZStd::string_view propertyId);

            //! Tokenizes an ID string like "itemA.itemB.itemC" into a vector like ["itemA", "itemB", "itemC"].
            static AZStd::vector<AZStd::string_view> TokenizeId(AZStd::string_view id);
            
            //! Splits an ID string like "itemA.itemB.itemC" into a vector like ["itemA.itemB", "itemC"].
            static AZStd::vector<AZStd::string_view> SplitId(AZStd::string_view id);

            //! Describes a path in the hierarchy of property groups, with the top level group at the beginning and a leaf-most group at the end.
            using PropertyGroupStack = AZStd::vector<const PropertyGroup*>;

            //! Call back function type used with the enumeration functions.
            //! The PropertyGroupStack contains the stack of property groups at the current point in the traversal.
            //! Return false to terminate the traversal.
            using EnumeratePropertyGroupsCallback = AZStd::function<bool(const PropertyGroupStack&)>;

            //! Recursively traverses all of the property groups contained in the material type, executing a callback function for each.
            //! @return false if the enumeration was terminated early by the callback returning false.
            bool EnumeratePropertyGroups(const EnumeratePropertyGroupsCallback& callback) const;

            //! Call back function type used with the numeration functions.
            //! Return false to terminate the traversal.
            using EnumeratePropertiesCallback = AZStd::function<bool(
                const PropertyDefinition*, // the property definition object 
                const MaterialNameContext& // The name context that the property is in, used to scope properties and shader connections (i.e. "levelA.levelB.")
                )>;
            
            //! Recursively traverses all of the properties contained in the material type, executing a callback function for each.
            //! @return false if the enumeration was terminated early by the callback returning false.
            bool EnumerateProperties(const EnumeratePropertiesCallback& callback) const;

            //! Returns a MaterialNameContext for a specific path through the property group hierarchy.
            static MaterialNameContext MakeMaterialNameContext(const MaterialTypeSourceData::PropertyGroupStack& propertyGroupStack);

            //! Create a MaterialTypeAsset for use at runtime. This is only valid for material types with the "direct" format (see GetFormat()).
            Outcome<Data::Asset<MaterialTypeAsset>> CreateMaterialTypeAsset(Data::AssetId assetId, AZStd::string_view materialTypeSourceFilePath = "", bool elevateWarnings = true) const;

            //! If the data was loaded from the legacy format file (i.e. where "groups" and "properties" were separate sections),
            //! this converts to the new format where properties are listed inside property groups.
            bool UpgradeLegacyFormat();

            //! See enum Format
            Format GetFormat() const;

        private:
                
            const PropertyGroup* FindPropertyGroup(AZStd::span<const AZStd::string_view> parsedPropertyGroupId, AZStd::span<const AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList) const;
            PropertyGroup* FindPropertyGroup(AZStd::span<AZStd::string_view> parsedPropertyGroupId, AZStd::span<AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList);
            
            const PropertyDefinition* FindProperty(AZStd::span<const AZStd::string_view> parsedPropertyId, AZStd::span<const AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList) const;
            PropertyDefinition* FindProperty(AZStd::span<AZStd::string_view> parsedPropertyId, AZStd::span<AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList);
            
            // Function overloads for recursion, returns false to indicate that recursion should end.
            bool EnumeratePropertyGroups(const EnumeratePropertyGroupsCallback& callback, PropertyGroupStack& propertyGroupStack, const AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& inPropertyGroupList) const;
            bool EnumerateProperties(const EnumeratePropertiesCallback& callback, MaterialNameContext nameContext, const AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& inPropertyGroupList) const;
            
            static void ExtendNameContext(MaterialNameContext& nameContext, const MaterialTypeSourceData::PropertyGroup& propertyGroup);

            //! Resolve source values (e.g. image filename, Enum string) to their asset version
            //! (ImageAsset, uint32_t).
            static MaterialPropertyValue ResolveSourceValue(
                const Name& propertyId,
                const MaterialPropertyValue& sourceValue,
                const AZStd::string& materialTypeSourceFilePath,
                const MaterialPropertiesLayout* materialPropertiesLayout,
                AZStd::function<void(const char*)> onError);

            //! Recursively populates a material type asset with properties from the tree of material property groups.
            //! @param materialTypeSourceFilePath path to the material type file that is being processed, used to look up relative paths
            //! @param materialTypeAssetCreator properties will be added to this creator
            //! @param materialNameContext the accumulated name context that should be applied to any property names or connection names encountered in the current @propertyGroup
            //! @param propertyGroup the current PropertyGroup that is being processed
            //! @return false if errors are detected and processing should abort
            bool BuildPropertyList(
                const AZStd::string& materialTypeSourceFilePath,
                MaterialTypeAssetCreator& materialTypeAssetCreator,
                MaterialNameContext materialNameContext,
                const MaterialTypeSourceData::PropertyGroup* propertyGroup) const;
                            
            //! Construct a complete list of group definitions, including implicit groups, arranged in the same order as the source data.
            //! Groups with the same name will be consolidated into a single entry.
            //! Operates on the old format PropertyLayout::m_groups, used for conversion to the new format.
            AZStd::vector<GroupDefinition> GetOldFormatGroupDefinitionsInDisplayOrder() const;
            
            PropertyLayout m_propertyLayout;
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

            Ptr<MaterialFunctorSourceData> GetActualSourceData() const { return m_actualSourceData; }
        private:
            Ptr<MaterialFunctorSourceData> m_actualSourceData = nullptr; // The derived material functor instance.
        };
    } // namespace RPI

} // namespace AZ
