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
#include <AzCore/std/containers/span.h>

#include <Atom/RPI.Public/AssetInitBus.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>

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
        //! Use a MaterialAssetCreator to create a MaterialAsset.
        class MaterialAsset
            : public AZ::Data::AssetData
            , public AssetInitBus::Handler
        {
            friend class MaterialVersionUpdates;
            friend class MaterialAssetCreator;
            friend class MaterialAssetHandler;
            friend class UnitTest::MaterialTests;
            friend class UnitTest::MaterialAssetTests;

        public:
            AZ_RTTI(MaterialAsset, "{522C7BE0-501D-463E-92C6-15184A2B7AD8}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(MaterialAsset, SystemAllocator, 0);

            static const char* DisplayName;
            static const char* Group;
            static const char* Extension;
            
            static constexpr uint32_t UnspecifiedMaterialTypeVersion = static_cast<uint32_t>(-1);

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
            //!
            //! Calling GetPropertyValues() will automatically finalize the material asset if it isn't finalized already. The
            //! MaterialTypeAsset must be loaded and ready.
            const AZStd::vector<MaterialPropertyValue>& GetPropertyValues();
            
            //! Returns true if material was created in a finalize state, as opposed to being finalized after loading from disk.
            bool WasPreFinalized() const;
            
            //! Returns the list of raw values for all properties in this material, as listed in the source .material file(s), before the material asset was Finalized.
            //! 
            //! The MaterialAsset can be created in a "half-baked" state (see MaterialUtils::BuildersShouldFinalizeMaterialAssets) where
            //! minimal processing has been done because it did not yet have access to the MaterialTypeAsset. In that case, the list will
            //! be populated with values copied from the source .material file with little or no validation or other processing. It includes
            //! all parent .material files, with properties listed in low-to-high priority order. 
            //! This list will be empty however if the asset was finalized at build-time (i.e. WasPreFinalized() returns true).
            const AZStd::vector<AZStd::pair<Name, MaterialPropertyValue>>& GetRawPropertyValues() const;

        private:
            bool PostLoadInit() override;
            
            //! If the material asset is not finalized yet, this does the final processing of the raw property values to get the material asset ready to be used.
            //! MaterialTypeAsset must be valid before this is called.
            void Finalize(AZStd::function<void(const char*)> reportWarning = nullptr, AZStd::function<void(const char*)> reportError = nullptr);

            //! Checks the material type version and potentially applies a series of property
            //! changes based on the MaterialTypeAsset's version update procedure.
            void ApplyVersionUpdates(AZStd::function<void(const char*)> reportError);

            //! Called by asset creators to assign the asset to a ready state.
            void SetReady();

            static const char* s_debugTraceName;

            Data::Asset<MaterialTypeAsset> m_materialTypeAsset = { AZ::Data::AssetLoadBehavior::PreLoad };

            //! Holds values for each material property, used to initialize Material instances.
            //! This is indexed by MaterialPropertyIndex and aligns with entries in m_materialPropertiesLayout.
            mutable AZStd::vector<MaterialPropertyValue> m_propertyValues;

            //! The MaterialAsset can be created in a "half-baked" state where minimal processing has been done because it does
            //! not yet have access to the MaterialTypeAsset. In that case, this list will be populated with values copied from
            //! the source .material file with little or no validation or other processing, and the m_propertyValues list will be empty.
            //! Once a MaterialTypeAsset is available, Finalize() must be called to finish processing these values into the
            //! final m_propertyValues list.
            //! Note that the content of this list will remain after finalizing in order to support hot-reload of the MaterialTypeAsset.
            //! The reason we use a vector instead of a map is to ensure inherited property values are applied in the right order;
            //! if the material has a parent, and that parent uses an older material type version with renamed properties, then 
            //! m_rawPropertyValues could be holding two values for the same property under different names. The auto-rename process
            //! can't be applied until the MaterialTypeAsset is available, so we have to keep the properties in the same order they
            //! were originally encountered.
            AZStd::vector<AZStd::pair<Name, MaterialPropertyValue>> m_rawPropertyValues;
            
            //! Tracks whether Finalize() has been called, meaning m_propertyValues is populated with data matching the material type's property layout.
            //! (This value is intentionally not serialized, it is set by the Finalize() function)
            mutable bool m_isFinalized = false;

            //! Tracks whether the MaterialAsset was already in a finalized state when it was loaded.
            bool m_wasPreFinalized = false;
            
            //! The materialTypeVersion this materialAsset was based off. If the versions do not match at runtime when a
            //! materialTypeAsset is loaded, automatic updates will be attempted.
            uint32_t m_materialTypeVersion = UnspecifiedMaterialTypeVersion;
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
