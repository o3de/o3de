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
#include <AtomCore/std/containers/array_view.h>

#include <Atom/RPI.Public/AssetInitBus.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>
#include <Atom/RPI.Public/Material/MaterialReloadNotificationBus.h>

#include <AzCore/EBus/Event.h>

namespace UnitTest
{
    class MaterialTests;
    class MaterialAssetTests;
}

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
            friend class MaterialVersionUpdate;
            friend class MaterialAssetCreator;
            friend class MaterialAssetHandler;
            friend class MaterialAssetCreatorCommon;
            friend class UnitTest::MaterialTests;
            friend class UnitTest::MaterialAssetTests;

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

            //! Returns the shader resource group layout that has per-material frequency, which indicates most of the topology
            //! for a material's shaders.
            //! All shaders in a material will have the same per-material SRG layout.
            //! @param supervariantIndex: supervariant index to get the layout from.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetMaterialSrgLayout(const SupervariantIndex& supervariantIndex) const;

            //! Same as above but accepts the supervariant name. There's a minor penalty when using this function
            //! because it will discover the index from the name.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetMaterialSrgLayout(const AZ::Name& supervariantName) const;

            //! Just like the original GetMaterialSrgLayout() where it uses the index of the default supervariant.
            //! See the definition of DefaultSupervariantIndex.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetMaterialSrgLayout() const;

            //! Returns the shader resource group layout that has per-object frequency. What constitutes an "object" is an
            //! agreement between the FeatureProcessor and the shaders, but an example might be world-transform for a model.
            //! All shaders in a material will have the same per-object SRG layout.
            //! @param supervariantIndex: supervariant index to get the layout from.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetObjectSrgLayout(const SupervariantIndex& supervariantIndex) const;

            //! Same as above but accepts the supervariant name. There's a minor penalty when using this function
            //! because it will discover the index from the name.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetObjectSrgLayout(const AZ::Name& supervariantName) const;

            //! Just like the original GetObjectSrgLayout() where it uses the index of the default supervariant.
            //! See the definition of DefaultSupervariantIndex.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& GetObjectSrgLayout() const;

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

            //! Realigns property value and name indices with MaterialProperiesLayout by using m_propertyNames. Property names not found in the
            //! MaterialPropertiesLayout are discarded, while property names not included in m_propertyNames will use the default value
            //! from m_materialTypeAsset.
            void RealignPropertyValuesAndNames();

            //! Checks the material type version and potentially applies a series of property changes (most common are simple property renames)
            //! based on the MaterialTypeAsset's version update procedure.
            void ApplyVersionUpdates();

            //! Called by asset creators to assign the asset to a ready state.
            void SetReady();

            // AssetBus overrides...
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;

            //! Replaces the MaterialTypeAsset when a reload occurs
            void ReinitializeMaterialTypeAsset(Data::Asset<Data::AssetData> asset);

            // MaterialReloadNotificationBus overrides...
            void OnMaterialTypeAssetReinitialized(const Data::Asset<MaterialTypeAsset>& materialTypeAsset) override;

            static const char* s_debugTraceName;

            Data::Asset<MaterialTypeAsset> m_materialTypeAsset;

            //! Holds values for each material property, used to initialize Material instances.
            //! This is indexed by MaterialPropertyIndex and aligns with entries in m_materialPropertiesLayout.
            AZStd::vector<MaterialPropertyValue> m_propertyValues;
            //! This is used to realign m_propertyValues as well as itself with MaterialPropertiesLayout when not empty.
            //! If empty, this implies that m_propertyValues is aligned with the entries in m_materialPropertiesLayout.
            AZStd::vector<AZ::Name> m_propertyNames;

            //! The materialTypeVersion this materialAsset was based of. If the versions do not match at runtime when a
            //! materialTypeAsset is loaded, an update will be performed on m_propertyNames if populated. 
            uint32_t m_materialTypeVersion = 1;

            //! A flag to determine if m_propertyValues needs to be aligned with MaterialPropertiesLayout. Set to true whenever
            //! m_materialTypeAsset is reinitializing.
            bool m_isDirty = true;
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
