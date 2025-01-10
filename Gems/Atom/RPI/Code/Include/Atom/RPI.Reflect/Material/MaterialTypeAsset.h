/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/any.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/std/containers/span.h>

#include <Atom/RPI.Public/AssetInitBus.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class MaterialTypeAssetHandler;

        class ATOM_RPI_REFLECT_API UvNamePair final
        {
        public:
            AZ_RTTI(UvNamePair, "{587D2902-B236-41B6-8F7B-479D891CC3F3}");
            static void Reflect(ReflectContext* context);

            UvNamePair() = default;
            UvNamePair(RHI::ShaderSemantic shaderInput, Name uvName) : m_shaderInput(shaderInput), m_uvName(uvName) {}

            RHI::ShaderSemantic m_shaderInput;
            Name m_uvName;
        };
        using MaterialUvNameMap = AZStd::vector<UvNamePair>;

        static const Name MaterialPipelineNone = Name{};

        //! MaterialTypeAsset defines the property layout and general behavior for 
        //! a type of material. It serves as the foundation for MaterialAssets,
        //! which can be used to render meshes at runtime.
        //! 
        //! Use a MaterialTypeAssetCreator to create a MaterialTypeAsset.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API MaterialTypeAsset
            : public AZ::Data::AssetData
            , public Data::AssetBus::MultiHandler
            , public AssetInitBus::Handler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class MaterialTypeAssetCreator;
            friend class MaterialTypeAssetHandler;

        public:
            AZ_RTTI(MaterialTypeAsset, "{CD7803AB-9C4C-4A33-9A14-7412F1665464}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(MaterialTypeAsset, SystemAllocator);

            static constexpr const char* DisplayName{ "MaterialTypeAsset" };
            static constexpr const char* Group{ "Material" };
            static constexpr const char* Extension{ "azmaterialtype" };

            static constexpr AZ::u32 SubId = 0;

            static constexpr uint32_t InvalidShaderIndex = static_cast<uint32_t>(-1);

            //! Provides data about how to render the material in a particular render pipeline.
            struct MaterialPipelinePayload
            {
                AZ_TYPE_INFO(MaterialTypeAsset::MaterialPipelinePayload, "{7179B076-70B6-4B47-9F98-BEF164396873}");

                Ptr<MaterialPropertiesLayout> m_materialPropertiesLayout;     //!< The layout of internal properties that the material type can use to configure this MaterialPipelinePayload
                AZStd::vector<MaterialPropertyValue> m_defaultPropertyValues; //!< Default values for each of the internal properties.
                ShaderCollection m_shaderCollection;                          //!< The collection of shaders that target the particular render pipeline.
                MaterialFunctorList m_materialFunctors;                       //!< These material functors consume data from the internal properties and configure the shader collection.
            };
            using MaterialPipelineMap = AZStd::unordered_map<Name, MaterialPipelinePayload>;

            static void Reflect(ReflectContext* context);

            virtual ~MaterialTypeAsset();

            //! Return the general purpose shader collection that applies to any render pipeline.
            const ShaderCollection& GetGeneralShaderCollection() const;

            //! The material may contain any number of MaterialFunctors.
            //! Material functors provide custom logic and calculations to configure shaders, render states, and more.
            //! See MaterialFunctor.h for details.
            const MaterialFunctorList& GetMaterialFunctors() const;

            //! Return the collection of MaterialPipelinePayload data for all supported material pipelines.
            const MaterialPipelineMap& GetMaterialPipelinePayloads() const;

            //! Returns the shader resource group layout that has per-material frequency, which indicates most of the topology
            //! for a material's shaders.
            //! All shaders in a material will have the same per-material SRG layout.
            //! @param supervariantIndex: supervariant index to get the layout from.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetMaterialSrgLayout(const SupervariantIndex& supervariantIndex) const;

            //! Same as above but accepts the supervariant name. There's a minor penalty when using this function
            //! because it will discover the index from the name.  
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetMaterialSrgLayout(const Name& supervariantName) const;

            //! Just like the original GetMaterialSrgLayout() where it uses the index of the default supervariant.
            //! See the definition of DefaultSupervariantIndex.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetMaterialSrgLayout() const;

            //! Returns a ShaderAsset from @m_shaderCollection that contains the MaterialSrg layout.
            const Data::Asset<ShaderAsset>& GetShaderAssetForMaterialSrg() const;

            //! Returns the shader resource group layout that has per-object frequency. What constitutes an "object" is an
            //! agreement between the FeatureProcessor and the shaders, but an example might be world-transform for a model.
            //! All shaders in a material will have the same per-object SRG layout.
            //! @param supervariantIndex: supervariant index to get the layout from.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetObjectSrgLayout(const SupervariantIndex& supervariantIndex) const;

            //! Same as above but accepts the supervariant name. There's a minor penalty when using this function
            //! because it will discover the index from the name.  
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetObjectSrgLayout(const Name& supervariantName) const;

            //! Just like the original GetObjectSrgLayout() where it uses the index of the default supervariant.
            //! See the definition of DefaultSupervariantIndex.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetObjectSrgLayout() const;

            //! Returns a ShaderAsset from @m_shaderCollection that contains the ObjectSrg layout.
            const Data::Asset<ShaderAsset>& GetShaderAssetForObjectSrg() const;

            //! Returns a layout that includes a list of MaterialPropertyDescriptors for each material property.
            const MaterialPropertiesLayout* GetMaterialPropertiesLayout() const;

            //! Returns the list of values for all properties in this material.
            //! The entries in this list align with the entries in the MaterialPropertiesLayout. Each AZStd::any is guaranteed 
            //! to have a value of type that matches the corresponding MaterialPropertyDescriptor.
            //! For images, the value will be of type ImageBinding.
            const AZStd::vector<MaterialPropertyValue>& GetDefaultPropertyValues() const;

            //! Returns a map from the UV shader inputs to a custom name.
            MaterialUvNameMap GetUvNameMap() const;

            //! Returns the version of the MaterialTypeAsset.
            uint32_t GetVersion() const;
 
            const MaterialVersionUpdates& GetMaterialVersionUpdates() const { return m_materialVersionUpdates; }

            //! Possibly renames @propertyId based on the material version update steps.
            //! @return true if the property was renamed
            bool ApplyPropertyRenames(Name& propertyId) const;

            bool InitializeNonSerializedData();

        private:

            bool PostLoadInit() override;

            //! Called by asset creators to assign the asset to a ready state.
            void SetReady();

            // AssetBus overrides...
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;

            //! Replaces the appropriate asset members when a reload occurs
            void ReinitializeAsset(Data::Asset<Data::AssetData> asset);

            void ForAllShaderItems(AZStd::function<bool(const Name& materialPipelineName, ShaderCollection::Item& shaderItem, uint32_t shaderIndex)> callback);

            //! Holds values for each material property, used to initialize Material instances.
            //! This is indexed by MaterialPropertyIndex and aligns with entries in m_materialPropertiesLayout.
            AZStd::vector<MaterialPropertyValue> m_propertyValues;

            //! Override names of UV inputs in the shaders of this material type.
            MaterialUvNameMap m_uvNameMap;

            //! Defines the topology of user-facing inputs to the material
            Ptr<MaterialPropertiesLayout> m_materialPropertiesLayout;

            //! List of shaders that will be run in any render pipeline
            ShaderCollection m_generalShaderCollection;

            //! Material functors provide custom logic and calculations to configure shaders, render states, and more.
            //! See MaterialFunctor.h for details.
            MaterialFunctorList m_materialFunctors;

            //! Describes how to render the material in specific render pipelines
            MaterialPipelineMap m_materialPipelinePayloads;

            //! These are shaders that hold an example of particular ShaderResourceGroups. Every shader in a material type
            //! must use the same MaterialSrg and ObjectSrg, so we only need to store one example of each. We keep a reference
            //! to the shader rather than duplicate the SRG layouts to avoid duplication and also because the ShaderAsset
            //! is needed to create an instance of the SRG so it's convenient to just keep a reference to the ShaderAsset.
            Data::Asset<ShaderAsset> m_shaderWithMaterialSrg;
            Data::Asset<ShaderAsset> m_shaderWithObjectSrg;

            //! The version of this MaterialTypeAsset. If the version is greater than 1, actions performed
            //! to update this MaterialTypeAsset will be in m_materialVersionUpdateMap
            uint32_t m_version = 1;

            //! Contains actions to perform for each material update version.
            MaterialVersionUpdates m_materialVersionUpdates;

            bool m_isNonSerializedDataInitialized = false;
        };

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API MaterialTypeAssetHandler : public AssetHandler<MaterialTypeAsset>
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            using Base = AssetHandler<MaterialTypeAsset>;
        public:
            AZ_RTTI(MaterialTypeAssetHandler, "{08568C59-CB7A-4F8F-AFCD-0B69F645B43F}", Base);

            AZ::Data::AssetHandler::LoadResult LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        };
    }
}
