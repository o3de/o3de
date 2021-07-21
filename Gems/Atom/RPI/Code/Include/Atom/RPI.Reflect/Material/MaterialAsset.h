/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AtomCore/std/containers/array_view.h>

#include <Atom/RPI.Public/AssetInitBus.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>
#include <Atom/RPI.Public/Material/MaterialReloadNotificationBus.h>

#include <AzCore/EBus/Event.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class MaterialAssetHandler;

        //! MaterialAsset defines a single material, which can be used to create a Material instance for rendering at runtime.
        //! It fetches MaterialTypeSourceData from the MaterialTypeAsset it owned.
        //! 
        //! Use a MaterialAssetCreator to create a MaterialAsset.
        class MaterialAsset
            : public AZ::Data::AssetData
            , public Data::AssetBus::Handler
            , public MaterialReloadNotificationBus::Handler
            , public AssetInitBus::Handler
        {
            friend class MaterialAssetCreator;
            friend class MaterialAssetHandler;
            friend class MaterialAssetCreatorCommon;

        public:
            AZ_RTTI(MaterialAsset, "{522C7BE0-501D-463E-92C6-15184A2B7AD8}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(MaterialAsset, SystemAllocator, 0);

            static const char* DisplayName;
            static const char* Group;
            static const char* Extension;

            static void Reflect(ReflectContext* context);

            MaterialAsset();
            virtual ~MaterialAsset();

            //! Returns the MaterialTypeAsset.
            const Data::Asset<MaterialTypeAsset>& GetMaterialTypeAsset() const;

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
            //!
            //! Note that even though material source data files contain only override values and inherit the rest from
            //! their parent material, they all get flattened at build time so every MaterialAsset has the full set of values.
            AZStd::array_view<MaterialPropertyValue> GetPropertyValues() const;

        private:
            bool PostLoadInit() override;

            //! Called by asset creators to assign the asset to a ready state.
            void SetReady();

            // AssetBus overrides...
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;

            //! Replaces the MaterialTypeAsset when a reload occurs
            void ReinitializeMaterialTypeAsset(Data::Asset<Data::AssetData> asset);

            // MaterialReloadNotificationBus overrides...
            void OnMaterialTypeAssetReinitialized(const Data::Asset<MaterialTypeAsset>& materialTypeAsset) override;

            Data::Asset<MaterialTypeAsset> m_materialTypeAsset;

            //! Holds values for each material property, used to initialize Material instances.
            //! This is indexed by MaterialPropertyIndex and aligns with entries in m_materialPropertiesLayout.
            AZStd::vector<MaterialPropertyValue> m_propertyValues;
        };
       

        class MaterialAssetHandler : public AssetHandler<MaterialAsset>
        {
            using Base = AssetHandler<MaterialAsset>;
        public:
            AZ_RTTI(MaterialAssetHandler, "{949DFEF5-6E19-4C81-8CF0-C764F474CCDD}", Base);

            Data::AssetHandler::LoadResult LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        };

    } // namespace RPI
} // namespace AZ
