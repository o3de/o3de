/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialAssetCreatorCommon.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <AtomCore/std/containers/array_view.h>

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
        class MaterialTypeAssetCreator
            : public AssetCreator<MaterialTypeAsset>
            , public MaterialAssetCreatorCommon
        {
        public:
            //! Begin creating a MaterialTypeAsset
            void Begin(const Data::AssetId& assetId);

            //! Adds a shader to the built-in shader collection, which will be run for this material.
            //! shaderTag must be unique within the material type's list of shaders.
            void AddShader(const AZ::Data::Asset<ShaderAsset>& shaderAsset, const ShaderVariantId& shaderVaraintId = ShaderVariantId{}, const AZ::Name& shaderTag = Uuid::CreateRandom().ToString<AZ::Name>());
            void AddShader(const AZ::Data::Asset<ShaderAsset>& shaderAsset, const AZ::Name& shaderTag);

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
            //! the property will not appear in GetMaterialPropertiesLayout() until EndMaterialProperty() is called.
            void BeginMaterialProperty(const Name& materialPropertyName, MaterialPropertyDataType dataType);
            
            //! Adds an output mapping from the current material property to a ShaderResourceGroup input.
            void ConnectMaterialPropertyToShaderInput(const Name& shaderInputName);
            
            //! Adds an output mapping from the current material property to a shader option in a specific shader.
            //! @param shaderIndex  Index to the material type's list of shader references, according to AddShader().
            void ConnectMaterialPropertyToShaderOption(const Name& shaderOptionName, uint32_t shaderIndex);

            //! Adds output mappings from the current material property to a shader option in multiple shaders.
            //! Will add one mapping for every ShaderAsset that has a matching shader option.
            void ConnectMaterialPropertyToShaderOptions(const Name& shaderOptionName);

            //! Store the enum names if a property is an enum type.
            void SetMaterialPropertyEnumNames(const AZStd::vector<AZStd::string>& enumNames);

            //! Finishes creating a material property.
            void EndMaterialProperty();

            //! Adds a MaterialFunctor.
            //! Material functors provide custom logic and calculations to configure shaders, render states, and more.See MaterialFunctor.h for details.
            void AddMaterialFunctor(const Ptr<MaterialFunctor>& functor);

            //! Adds UV name for a shader input.
            void AddUvName(const RHI::ShaderSemantic& shaderInput, const Name& uvName);

            //! This provides access to the MaterialPropertiesLayout while the MaterialTypeAsset is still being built.
            //! This is needed by MaterialTypeSourceData to initialize functor objects.
            //! @return A valid pointer when called between Begin() and End(). Otherwise, returns nullptr.
            const MaterialPropertiesLayout* GetMaterialPropertiesLayout() const;

            //! This provides access to the material ShaderResourceGroupLayout being used for the MaterialTypeAsset.
            //! This is needed by MaterialTypeSourceData to initialize functor objects.
            //! The same layout object can be retrieved from the ShaderAssets passed to the creator, but this function 
            //! is provided for convenience.
            //! @return A valid pointer if a ShaderAsset with a material ShaderResourceGroupLayout was added. Otherwise, returns nullptr.
            const RHI::ShaderResourceGroupLayout* GetMaterialShaderResourceGroupLayout() const;

            //! This provides access to the ShaderCollection while the MaterialAsset is still being built.
            //! This is needed by MaterialTypeSourceData to initialize functor objects.
            //! @return A valid pointer when called between Begin() and End(). Otherwise, returns nullptr.
            const ShaderCollection* GetShaderCollection() const;

            bool End(Data::Asset<MaterialTypeAsset>& result);

        private:

            void AddMaterialProperty(MaterialPropertyDescriptor&& materialProperty);

            //! The material type holds references to shader assets that contain SRGs that are supposed to be the same across all passes in the material.
            //! This function searches for an SRG given a @bindingSlot. If a valid one is found it makes sure it is the same across all shaders
            //! and records in srgShaderIndexToUpdate the index of the ShaderAsset in the ShaderCollection where it was found.
            //! @srgShaderIndexToUpdate Previously found shader index. If Invalid, this will be filled with the index of the shader Asset
            //!     where the bindingSlot was found. If not Invalid, this will validate that the same SRG is used by
            //!                    newShaderAsset.
            //! @newShaderAssetIndex Corresponding index of @newShaderAsset in m_asset->m_shaderCollection.
            //! @bindingSlot  The binding slot ID of the SRG to fetch from newShaderAsset.
            bool UpdateShaderIndexForShaderResourceGroup(
                uint32_t& srgShaderIndexToUpdate, const Data::Asset<ShaderAsset>& newShaderAsset, const uint32_t newShaderAssetIndex,
                const uint32_t bindingSlot, const char* srgDebugName);

            //! Saves the per-material SRG layout in m_shaderResourceGroupLayout for easier access
            void CacheMaterialSrgLayout();
            
            bool ValidateMaterialVersion();
            bool ValidateBeginMaterialProperty();
            bool ValidateEndMaterialProperty();

            MaterialPropertiesLayout* m_materialPropertiesLayout = nullptr; //!< Cached pointer to the MaterialPropertiesLayout being created
            const RHI::ShaderResourceGroupLayout* m_materialShaderResourceGroupLayout = nullptr; //!< The per-material ShaderResourceGroup layout
            MaterialPropertyDescriptor m_wipMaterialProperty; //!< Material property being created. Is valid between BeginMaterialProperty() and EndMaterialProperty()
        };

    } // namespace RPI
} // namespace AZ
