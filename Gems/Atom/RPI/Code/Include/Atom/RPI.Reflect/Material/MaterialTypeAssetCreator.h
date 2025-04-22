/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <AzCore/std/containers/span.h>

namespace AZ
{
    namespace RPI
    {
        class StreamingImageAsset;
        class AttachmentImageAsset;

        //! Use a MaterialAssetCreator to create and configure a new MaterialAsset.
        //! The MaterialAsset will be based on a MaterialTypeAsset, which provides the 
        //! necessary data to define the layout and behavior of the material. The 
        //! MaterialAsset itself only provides property values.
        //! The MaterialAsset may optionally inherit from another 'parent' MaterialAsset,
        //! which provides the MaterialTypeAsset and default property values.
        class ATOM_RPI_REFLECT_API MaterialTypeAssetCreator
            : public AssetCreator<MaterialTypeAsset>
        {
        public:

            //! Begin creating a MaterialTypeAsset
            void Begin(const Data::AssetId& assetId);

            //! Adds a shader to the built-in shader collection, which will be run for this material.
            //! @param shaderTag Must be unique within the material type's list of shaders.
            //! @param materialPipelineName Identifies a specific MaterialPipelinePayload that this shader should be added to.
            //!                             For MaterialPipelineNone, the shader will be used for all pipelines.
            void AddShader(
                const AZ::Data::Asset<ShaderAsset>& shaderAsset,
                const ShaderVariantId& shaderVariantId = {},
                const AZ::Name& shaderTag = {},
                const AZ::Name& materialPipelineName = MaterialPipelineNone);

            //! Sets the version of the MaterialTypeAsset
            void SetVersion(uint32_t version);
            //! Adds a version update object into the MaterialTypeAsset
            void AddVersionUpdate(const MaterialVersionUpdate& materialVersionUpdate);

            //! Indicates that this MaterialType will own the specified shader option.
            //! Material-owned shader options can be connected to material properties (either directly or through functors).
            //! They cannot be accessed externally (for example, through the Material::SetSystemShaderOption() function).
            //! Note that ConnectMaterialPropertyToShaderOptions() automatically claims ownership; so ClaimShaderOptionOwnership()
            //! only needs to be called for options used by material functors.
            void ClaimShaderOptionOwnership(const Name& shaderOptionName);

            //! Starts creating a material property.
            //! Note that EndMaterialProperty() must be called before calling SetMaterialPropertyValue(). Similarly,
            //! The property will not appear in GetMaterialPropertiesLayout() until EndMaterialProperty() is called.
            //! @param materialPipelineName For internal material properties, this indicates which material pipeline the property is for.
            //!                             For main material properties, use MaterialPipelineNone.
            void BeginMaterialProperty(const Name& materialPropertyName, MaterialPropertyDataType dataType, const AZ::Name& materialPipelineName = MaterialPipelineNone);

            //! Adds an output mapping from the current material property to a ShaderResourceGroup input.
            void ConnectMaterialPropertyToShaderInput(const Name& shaderInputName);
            
            //! Adds output mappings from the current material property to a shader option in multiple shaders.
            //! Will add one mapping for every ShaderAsset that has a matching shader option.
            void ConnectMaterialPropertyToShaderOptions(const Name& shaderOptionName);

            //! Adds an output mapping from the current material property to a Enabled flag of a specific shader.
            //! @param shaderTag  The tag name that unique identifies a shader in the material type.
            void ConnectMaterialPropertyToShaderEnabled(const Name& shaderTag);

            //! Adds an output mapping from the current material property to an internal material property in a MaterialPipelinePayload.
            //! The current property must not be an internal property from any MaterialPipelinePayload.
            //! The target property must have already been added to the MaterialTypeAssetCreator.
            //! @param propertyName  The name of an internal material property to connect to.
            void ConnectMaterialPropertyToInternalProperty(const Name& propertyName);

            //! Store the enum names if a property is an enum type.
            void SetMaterialPropertyEnumNames(const AZStd::vector<AZStd::string>& enumNames);

            //! Finishes creating a material property.
            void EndMaterialProperty();

            //! @param materialPipelineName For internal material properties, this indicates which MaterialPipelinePlayload's property to update.
            //!                             For main material properties, use MaterialPipelineNone.
            void SetPropertyValue(const Name& name, const Data::Asset<ImageAsset>& imageAsset, const AZ::Name& materialPipelineName = MaterialPipelineNone);
            void SetPropertyValue(const Name& name, const Data::Asset<StreamingImageAsset>& imageAsset, const AZ::Name& materialPipelineName = MaterialPipelineNone);
            void SetPropertyValue(const Name& name, const Data::Asset<AttachmentImageAsset>& imageAsset, const AZ::Name& materialPipelineName = MaterialPipelineNone);

            //! Sets a property value using data in AZStd::variant-based MaterialPropertyValue. The contained data must match
            //! the data type of the property. For type Image, the value must be a Data::Asset<ImageAsset>.
            //! @param materialPipelineName For internal material properties, this indicates which MaterialPipelinePayload's property to update.
            //!                             For main material properties, use MaterialPipelineNone.
            void SetPropertyValue(const Name& name, const MaterialPropertyValue& value, const AZ::Name& materialPipelineName = MaterialPipelineNone);

            //! Adds a MaterialFunctor.
            //! Material functors provide custom logic and calculations to configure shaders, render states, and more.See MaterialFunctor.h for details.
            //! @param materialPipelineName Identifies a MaterialPipelinePayload that this functor should be added to. For MaterialPipelineNone, 
            //!                             the functor will be used for the main ShaderCollection that applies to all pipelines.
            void AddMaterialFunctor(const Ptr<MaterialFunctor>& functor, const AZ::Name& materialPipelineName = MaterialPipelineNone);

            //! Adds UV name for a shader input.
            void AddUvName(const RHI::ShaderSemantic& shaderInput, const Name& uvName);

            //! This provides access to the MaterialPropertiesLayout while the MaterialTypeAsset is still being built.
            //! This is needed by MaterialTypeSourceData to initialize functor objects.
            //! @param materialPipelineName For internal material properties, this indicates which MaterialPipelinePayload's property layout to query.
            //!                             For main material properties, use MaterialPipelineNone.
            //! @return A valid pointer when called between Begin() and End(). Otherwise, returns nullptr.
            const MaterialPropertiesLayout* GetMaterialPropertiesLayout(const AZ::Name& materialPipelineName = MaterialPipelineNone) const;

            //! This provides access to the material ShaderResourceGroupLayout being used for the MaterialTypeAsset.
            //! This is needed by MaterialTypeSourceData to initialize functor objects.
            //! The same layout object can be retrieved from the ShaderAssets passed to the creator, but this function 
            //! is provided for convenience.
            //! @return A valid pointer if a ShaderAsset with a material ShaderResourceGroupLayout was added. Otherwise, returns nullptr.
            const RHI::ShaderResourceGroupLayout* GetMaterialShaderResourceGroupLayout() const;

            bool End(Data::Asset<MaterialTypeAsset>& result);

        private:

            void AddMaterialProperty(MaterialPropertyDescriptor&& materialProperty, const AZ::Name& materialPipelineName);
            
            bool PropertyCheck(TypeId typeId, const Name& propertyName, const AZ::Name& materialPipelineName);
                
            //! The material type holds references to shader assets that contain SRGs that are supposed to be the same across all passes in the material.
            //! This function searches for an SRG given a @bindingSlot. If a valid one is found it makes sure it is the same across all shaders
            //! and records it in @srgShaderAssetToUpdate.
            //! @srgShaderAssetToUpdate Previously found shader asset with the desired SRG. If Invalid, this will be filled with the shader asset
            //!     where the bindingSlot was found. If not Invalid, this will validate that the same SRG is used by newShaderAsset.
            //! @bindingSlot  The binding slot ID of the SRG to fetch from newShaderAsset.
            bool UpdateShaderAssetForShaderResourceGroup(
                Data::Asset<ShaderAsset>& srgShaderAssetToUpdate,
                const Data::Asset<ShaderAsset>& newShaderAsset,
                const uint32_t bindingSlot,
                const char* srgDebugName);

            //! Saves the per-material SRG layout in m_shaderResourceGroupLayout for easier access
            void CacheMaterialSrgLayout();
            
            bool ValidateMaterialVersion();
            bool ValidateBeginMaterialProperty();
            bool ValidateEndMaterialProperty();

            MaterialTypeAsset::MaterialPipelinePayload& GetMaterialPipelinePayload(const AZ::Name& materialPipelineName);

            
            MaterialPropertiesLayout* m_materialPropertiesLayout = nullptr; //!< Cached pointer to the MaterialPropertiesLayout being created
            const RHI::ShaderResourceGroupLayout* m_materialShaderResourceGroupLayout = nullptr; //!< The per-material ShaderResourceGroup layout
            MaterialPropertyDescriptor m_wipMaterialProperty; //!< Material property being created. Is valid between BeginMaterialProperty() and EndMaterialProperty()
            Name m_wipMaterialPropertyPipeline; //!< Tracks which MaterialPipelinePayload the material property is being created for, if any.
        };

    } // namespace RPI
} // namespace AZ
