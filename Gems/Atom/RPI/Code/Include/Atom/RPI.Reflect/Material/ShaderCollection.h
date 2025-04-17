/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <AtomCore/std/containers/vector_set.h>
#include <Atom/RHI.Reflect/NameIdReflectionMap.h>
#include <Atom/RHI.Reflect/Handle.h>


namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! Collects the set of all possible shaders that a material could use at runtime,
        //! along with configuration that indicates how each shader should be used.
        //! Each shader item may be reconfigured at runtime, but items cannot be added
        //! or removed (this restriction helps simplify overall material system code,
        //! especially around material functors).
        class ATOM_RPI_REFLECT_API ShaderCollection
        {
            friend class MaterialTypeAssetCreator;
        public:
            AZ_TYPE_INFO(ShaderCollection, "{8D509258-E32C-4CC7-AADC-D13F790DCE4B}");
            static void Reflect(AZ::ReflectContext* context);

            //! Contains shader asset and configures how that shader should be used
            //! at runtime, especially which variant of the shader to use.
            class ATOM_RPI_REFLECT_API Item
            {
                friend class MaterialTypeAssetCreator;
                friend class ShaderVariantReferenceSerializationEvents;
                friend class ShaderCollection;
                friend class MaterialTypeAsset;
            public:
                AZ_TYPE_INFO(Item, "{64C7F381-3313-46E8-B23B-D7AA9A915F35}");
                static void Reflect(AZ::ReflectContext* context);

                //! Required for use in containers; not meant to be called directly.
                Item();

                //! @param shaderAsset The ShaderAsset represented by this item.
                //! @param shaderTag Unique tag to identify this item.
                //! @param variantId The the initial state of shader option values for use with this shader item.
                Item(const Data::Asset<ShaderAsset>& shaderAsset, const AZ::Name& shaderTag, ShaderVariantId variantId = ShaderVariantId{});
                Item(Data::Asset<ShaderAsset>&& shaderAsset, const AZ::Name& shaderTag, ShaderVariantId variantId = ShaderVariantId{});

                const Data::Asset<ShaderAsset>& GetShaderAsset() const;

                //! Return the ID of the shader variant to be used, based on the configured shader options.
                const ShaderVariantId& GetShaderVariantId() const;

                //! Returns the asset id associated to this shader item.
                const AZ::Data::AssetId& GetShaderAssetId() const;

                const AZ::RPI::ShaderOptionGroup& GetShaderOptionGroup() const;

                //! Return the set of shader options used to select a specific shader variant.
                const ShaderOptionGroup* GetShaderOptions() const;
                ShaderOptionGroup* GetShaderOptions();

                //! Returns whether the material owns the indicate shader option in this Item.
                //! Material-owned shader options can be connected to material properties (either directly or through functors).
                //! They cannot be accessed externally (for example, through the Material::SetSystemShaderOption() function).
                bool MaterialOwnsShaderOption(const AZ::Name& shaderOptionName) const;
                bool MaterialOwnsShaderOption(ShaderOptionIndex shaderOptionIndex) const;

                //! Return the runtime render states overlay. Properties that are not overwritten are invalid.
                const RHI::RenderStates* GetRenderStatesOverlay() const;
                RHI::RenderStates* GetRenderStatesOverlay();

                //! Return the runtime draw list tag override.
                RHI::DrawListTag GetDrawListTagOverride() const;
                //! Set the runtime draw list tag.
                void SetDrawListTagOverride(RHI::DrawListTag drawList);
                void SetDrawListTagOverride(const AZ::Name& drawListName);

                //! Controls whether this shader/pass will be used for rendering a material.
                void SetEnabled(bool enabled);
                bool IsEnabled() const;

                //! Returns the shader tag used to identify this item
                const AZ::Name& GetShaderTag() const;

                //! If the AssetId of @newShaderAsset matches the AssetId of @m_shaderAsset,
                //! then @m_shaderAsset will be updated to @newShaderAsset, AND m_shaderOptionGroup
                //! will be updated too.
                void TryReplaceShaderAsset(const Data::Asset<ShaderAsset>& newShaderAsset);

                // Returns true if was able to initialized the non-serialized @m_shaderOptionGroup.
                // Only returns false if @m_shaderAsset is not ready.
                bool InitializeShaderOptionGroup();

            private:
                Data::Asset<ShaderAsset> m_shaderAsset;
                ShaderVariantId m_shaderVariantId;       //!< Temporarily holds the ShaderVariantId, used for serialization. This will be copied to/from m_shaderOptionGroup.
                ShaderOptionGroup m_shaderOptionGroup;   //!< Holds and manipulates the ShaderVariantId at runtime.
                RHI::RenderStates m_renderStatesOverlay; //!< Holds and manipulates the RenderStates at runtime.
                RHI::DrawListTag  m_drawListTagOverride; //!< Holds and manipulates the DrawList at runtime.
                //[GFX TODO][ATOM-5636]: This may need to use a more efficient data structure. Consider switching to vector_set class (which will need to be updated to support serialization).
                AZStd::unordered_set<ShaderOptionIndex> m_ownedShaderOptionIndices; //!< Set of shader options in this shader that are owned by the material.
                bool m_enabled = true;                   //!< Disabled items will not be included in the final draw packet that gets sent to the renderer.
                AZ::Name m_shaderTag;                    //!< Unique tag that identifies this item
            };

            using iterator = AZStd::vector<Item>::iterator;
            using const_iterator = AZStd::vector<Item>::const_iterator;

            size_t size() const;
            iterator begin();
            const_iterator begin() const;
            iterator end();
            const_iterator end() const;
            Item& operator[](size_t i);
            const Item& operator[](size_t i) const;

            bool HasShaderTag(const AZ::Name& shaderTag) const;
            Item& operator[](const AZ::Name& shaderTag);
            const Item& operator[](const AZ::Name& shaderTag) const;

            //! Convenience function that loops through all @m_shaderItems
            //! and calls TryReplaceShaderAsset on all of them.
            void TryReplaceShaderAsset(const Data::Asset<ShaderAsset>& newShaderAsset);

            //! Loops through all items in the collection and calls Item::InitializeShaderOptionGroup().
            //! Returns true if all Item::InitializeShaderOptionGroup() return true,
            //! otherwise returns false.
            bool InitializeShaderOptionGroups();

        private:
            using NameReflectionMapForIndex = RHI::NameIdReflectionMap<RHI::Handle<uint32_t>>;

            AZStd::vector<Item> m_shaderItems;
            NameReflectionMapForIndex m_shaderTagIndexMap;
        };

    } // namespace RPI
} // namespace AZ
