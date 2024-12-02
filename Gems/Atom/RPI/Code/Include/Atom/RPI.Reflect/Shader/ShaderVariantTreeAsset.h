/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderOptionGroupLayout;
        struct ShaderVariantId;
        struct ShaderVariantTreeNode;


        //! The shader variant tree is a data structure to perform lookups of shader variants that have the best runtime performance on the GPU.
        //! The tree supports the lookup of a best-fit shader variant, given a specific shader variant key.
        //! The best-fit variant should have the best runtime performance on the GPU, as it has less dynamic branches.
        //!
        //! The algorithm does the following:
        //! - Find a list of all matches for the specified shader variant ID.
        //! - Select the best variant from that list.
        //!
        //! The variant searched using the tree has a key that matches the requested key, but some values can be undefined.
        //! For example, requesting a key equal to "00101" could return a variant with ID "0?10?", in which ? stands for undefined values.
        //! The undefined values must be provided to the fallback constant buffer. (See Shader::FindFallbackShaderResourceGroupAsset).
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API ShaderVariantTreeAsset final
            : public Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ShaderVariantTreeAssetHandler;
            friend class ShaderVariantTreeAssetCreator;
            friend struct ShaderVariantTreeNode;

        public:
            AZ_CLASS_ALLOCATOR(ShaderVariantTreeAsset , SystemAllocator)
            AZ_RTTI(ShaderVariantTreeAsset, "{EBF48506-F8BB-4B37-8FAC-F132BF83E42D}", Data::AssetData);

            static void Reflect(ReflectContext* context);

            static constexpr const char* Extension = "azshadervarianttree";
            static constexpr const char* DisplayName = "ShaderVariantTree";
            static constexpr const char* Group = "Shader";
            static constexpr AZ::u32 ProductSubID = 0; //Reserved for ShaderVariantTreeAssets.

            //! See comments in ValidateShaderVariantListLocation() inside ShaderVariantAssetBuilder.cpp
            static constexpr char CommonSubFolder[] = "ShaderVariants";
            static constexpr char CommonSubFolderLowerCase[] = "shadervariants";

            ShaderVariantTreeAsset() = default;
            ~ShaderVariantTreeAsset() = default;

            AZ_DISABLE_COPY_MOVE(ShaderVariantTreeAsset);

            // A helper method. Given the assetId of a ShaderAsset it returns what should be the assetId of its corresponding
            // ShaderVariantTreeAsset. If the assetID doesn't exist in the database then it will return an invalid assetId.
            static Data::AssetId GetShaderVariantTreeAssetIdFromShaderAssetId(const Data::AssetId& shaderAssetId);


            //! Returns the total number of nodes.
            size_t GetNodeCount() const;

            //! Finds and returns the shader variant index associated with the specified ID.
            //! The search involves two general steps:
            //! - Search the tree to find all possible matches for the specified shader variant ID.
            //! - Search the best match from those results.
            ShaderVariantSearchResult FindVariantStableId(const ShaderOptionGroupLayout* shaderOptionGroupLayout, const ShaderVariantId& shaderVariantId) const;

        private:

            static constexpr uint32_t UnspecifiedIndex = std::numeric_limits<uint32_t>::max();

            //! Returns the node associated with the provided index.
            const ShaderVariantTreeNode& GetNode(uint32_t index) const;

            //! Sets the node associated with the provided index.
            void SetNode(uint32_t index, const ShaderVariantTreeNode& node);

            //! Build a list of values from the specified shader variant ID.
            static AZStd::vector<uint32_t> ConvertToValueChain(const ShaderOptionGroupLayout* shaderOptionGroupLayout, const ShaderVariantId& shaderVariantId);

            //! Called by asset creators to assign the asset to a ready state.
            void SetReady();
            bool FinalizeAfterLoad();

            //! We save here the hash of the ShaderAsset. When these hashes differs We will rebuild ALL ShaderVariantAssets.
            //! If the hash doesn't change, We will rebuild only the ShaderVariantAssets that changes or were added to the
            //! .shadervariantlist file.
            AZ::u64 m_shaderHash = 0;
            AZStd::vector<ShaderVariantTreeNode> m_nodes;
        };

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API ShaderVariantTreeAssetHandler final
            : public AssetHandler<ShaderVariantTreeAsset>
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            using Base = AssetHandler<ShaderVariantTreeAsset>;
        public:
            ShaderVariantTreeAssetHandler() = default;

        private:
            LoadResult LoadAssetData(const Data::Asset<Data::AssetData>& asset, AZStd::shared_ptr<Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
            bool PostLoadInit(const Data::Asset<Data::AssetData>& asset);
        };

        //! Helper structure for the nodes in the shader variant search tree.
        struct ATOM_RPI_REFLECT_API ShaderVariantTreeNode final
        {
        public:
            AZ_RTTI(ShaderVariantTreeNode, "{5C985619-B2AF-4761-937E-B66DB021637C}");

            static void Reflect(ReflectContext* context);

            ShaderVariantTreeNode();
            ShaderVariantTreeNode(const ShaderVariantStableId& index, uint32_t offset);

            //! Returns the index in the storage vector where the variant byte code can be found; otherwise returns -1.
            const ShaderVariantStableId& GetStableId() const;

            //! Returns the offset to the children nodes of this node; otherwise returns 0.
            uint32_t GetOffset() const;

            //! Checks if this node has children.
            bool HasChildren() const;

        private:
            ShaderVariantStableId m_stableId;
            uint32_t m_offset;
        };

    } // namespace RPI

} // namespace AZ
