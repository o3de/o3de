/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/any.h>
#include <AzCore/EBus/Event.h>
#include <AtomCore/std/containers/array_view.h>

#include <Atom/RPI.Public/AssetInitBus.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class MaterialTypeAssetHandler;

        class UvNamePair final
        {
        public:
            AZ_RTTI(AZ::RPI::MaterialTypeAsset, "{587D2902-B236-41B6-8F7B-479D891CC3F3}");
            static void Reflect(ReflectContext* context);

            UvNamePair() = default;
            UvNamePair(RHI::ShaderSemantic shaderInput, Name uvName) : m_shaderInput(shaderInput), m_uvName(uvName) {}

            RHI::ShaderSemantic m_shaderInput;
            Name m_uvName;
        };
        using MaterialUvNameMap = AZStd::vector<UvNamePair>;

        //! MaterialTypeAsset defines the property layout and general behavior for 
        //! a type of material. It serves as the foundation for MaterialAssets,
        //! which can be used to render meshes at runtime.
        //! 
        //! Use a MaterialTypeAssetCreator to create a MaterialTypeAsset.
        class MaterialTypeAsset
            : public AZ::Data::AssetData
            , public Data::AssetBus::MultiHandler
            , public AssetInitBus::Handler
        {
            friend class MaterialTypeAssetCreator;
            friend class MaterialTypeAssetHandler;
            friend class MaterialAssetCreatorCommon;

        public:
            AZ_RTTI(MaterialTypeAsset, "{CD7803AB-9C4C-4A33-9A14-7412F1665464}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(MaterialTypeAsset, SystemAllocator, 0);

            static const char* DisplayName;
            static const char* Group;
            static const char* Extension;

            static void Reflect(ReflectContext* context);

            virtual ~MaterialTypeAsset();

            //! Returns the collection of all shaders that this material could run.
            const ShaderCollection& GetShaderCollection() const;

            //! The material may contain any number of MaterialFunctors.
            //! Material functors provide custom logic and calculations to configure shaders, render states, and more.
            //! See MaterialFunctor.h for details.
            const MaterialFunctorList& GetMaterialFunctors() const;

            //! Returns the shader resource group asset that has per-material frequency, which indicates most of the topology 
            //! for a material's shaders.
            //! All shaders in a material will have the same per-material SRG asset.
            const Data::Asset<ShaderResourceGroupAsset>& GetMaterialSrgAsset() const;
            
            //! Returns the shader resource group asset that has per-object frequency. What constitutes an "object" is an
            //! agreement between the FeatureProcessor and the shaders, but an example might be world-transform for a model.
            //! All shaders in a material will have the same per-object SRG asset.
            const Data::Asset<ShaderResourceGroupAsset>& GetObjectSrgAsset() const;

            //! Returns a layout that includes a list of MaterialPropertyDescriptors for each material property.
            const MaterialPropertiesLayout* GetMaterialPropertiesLayout() const;

            //! Returns the list of values for all properties in this material.
            //! The entries in this list align with the entries in the MaterialPropertiesLayout. Each AZStd::any is guaranteed 
            //! to have a value of type that matches the corresponding MaterialPropertyDescriptor.
            //! For images, the value will be of type ImageBinding.
            AZStd::array_view<MaterialPropertyValue> GetDefaultPropertyValues() const;

            //! Returns a map from the UV shader inputs to a custom name.
            MaterialUvNameMap GetUvNameMap() const;

        private:
            bool PostLoadInit() override;

            //! Called by asset creators to assign the asset to a ready state.
            void SetReady();

            // AssetBus overrides...
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;

            //! Replaces the appropriate asset members when a reload occurs
            void ReinitializeAsset(Data::Asset<Data::AssetData> asset);

            //! Holds values for each material property, used to initialize Material instances.
            //! This is indexed by MaterialPropertyIndex and aligns with entries in m_materialPropertiesLayout.
            AZStd::vector<MaterialPropertyValue> m_propertyValues;

            //! Override names of UV inputs in the shaders of this material type.
            MaterialUvNameMap m_uvNameMap;

            //! Defines the topology of user-facing inputs to the material
            Ptr<MaterialPropertiesLayout> m_materialPropertiesLayout;

            //! The set of shaders that will be used for this material
            ShaderCollection m_shaderCollection;

            //! Material functors provide custom logic and calculations to configure shaders, render states, and more.
            //! See MaterialFunctor.h for details.
            MaterialFunctorList m_materialFunctors;

            //! Reference to the SRGs that are the same for all shaders in a material
            Data::Asset<ShaderResourceGroupAsset> m_materialSrgAsset;
            Data::Asset<ShaderResourceGroupAsset> m_objectSrgAsset;
        };

        class MaterialTypeAssetHandler : public AssetHandler<MaterialTypeAsset>
        {
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
