/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Reflect/Shader/ShaderVariantTreeAsset.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderVariantTreeAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantTreeAsset, AZ::Data::AssetData>()
                    ->Version(1)
                    ->Field("ShaderHash", &ShaderVariantTreeAsset::m_shaderHash)
                    ->Field("Nodes", &ShaderVariantTreeAsset::m_nodes)
                    ;
            }

            ShaderVariantTreeNode::Reflect(context);
        }

        Data::AssetId ShaderVariantTreeAsset::GetShaderVariantTreeAssetIdFromShaderAssetId(const Data::AssetId& shaderAssetId)
        {
            //From the shaderAssetId We can deduce the path of the shader asset, and from the path of the shader asset we can deduce the path of the ShaderVariantTreeAsset.
            AZ::IO::FixedMaxPath shaderAssetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(shaderAssetPath.Native(), &AZ::Data::AssetCatalogRequests::GetAssetPathById
                    , shaderAssetId);
            AZ::IO::FixedMaxPath shaderAssetPathRoot = shaderAssetPath.ParentPath();
            AZ::IO::FixedMaxPath shaderAssetPathName = shaderAssetPath.Stem();
            
            AZStd::string shaderVariantTreeAssetDir;
            AzFramework::StringFunc::Path::Join(ShaderVariantTreeAsset::CommonSubFolderLowerCase, shaderAssetPathRoot.c_str(), shaderVariantTreeAssetDir);
            AZStd::string shaderVariantTreeAssetFilename = AZStd::string::format("%s.%s", shaderAssetPathName.c_str(), ShaderVariantTreeAsset::Extension);
            AZStd::string shaderVariantTreeAssetPath;
            AzFramework::StringFunc::Path::Join(shaderVariantTreeAssetDir.c_str(), shaderVariantTreeAssetFilename.c_str(), shaderVariantTreeAssetPath);

            AZ::Data::AssetId shaderVariantTreeAssetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(shaderVariantTreeAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath
                , shaderVariantTreeAssetPath.c_str(), AZ::Data::s_invalidAssetType, false);

            if (!shaderVariantTreeAssetId.IsValid())
            {
                // If the game project did not customize the shadervariantlist, let's see if the original author of the .shader file
                // provided a shadervariantlist.
                AzFramework::StringFunc::Path::Join(shaderAssetPathRoot.c_str(), shaderVariantTreeAssetFilename.c_str(), shaderVariantTreeAssetPath);
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(shaderVariantTreeAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath
                    , shaderVariantTreeAssetPath.c_str(), AZ::Data::s_invalidAssetType, false);
            }

            return shaderVariantTreeAssetId;
        }

        size_t ShaderVariantTreeAsset::GetNodeCount() const
        {
            return m_nodes.size();
        }

        ShaderVariantSearchResult ShaderVariantTreeAsset::FindVariantStableId(const ShaderOptionGroupLayout* shaderOptionGroupLayout, const ShaderVariantId& shaderVariantId) const
        {
            struct NodeToVisit
            {
                uint32_t m_branchCount; // Number of static branches
                uint32_t m_nodeIndex;   // Index of the node to visit
            };

            struct SearchResult
            {
                uint32_t m_branchCount;            // Number of static branches
                ShaderVariantStableId m_variantStableId;
            };

            // The list of specified options, in order of priority, built from the variant key mask.
            auto optionValues = ConvertToValueChain(shaderOptionGroupLayout, shaderVariantId);

            // Always add the root to the results.
            AZStd::vector<SearchResult> searchResults;
            searchResults.push_back({ 0, ShaderAsset::RootShaderVariantStableId });

            // All the indices are guaranteed to be unique, so we use queues.
            AZStd::queue<NodeToVisit> nodesToVisit;
            AZStd::queue<NodeToVisit> nodesToVisitNext;

            // Always visit the root node.
            nodesToVisit.push({ 0, 0 });

            for (uint32_t optionValue : optionValues)
            {
                while (!nodesToVisit.empty())
                {
                    const NodeToVisit nextNode = nodesToVisit.front();
                    nodesToVisit.pop();

                    // Leaf node
                    if (!GetNode(nextNode.m_nodeIndex).HasChildren())
                    {
                        continue;
                    }

                    // Two branches need to be searched:
                    // - The node that is an exact match for the shader option value (specified).
                    // - The node that can match any shader option value (unspecified).

                    // The unspecified value node is always the first child.
                    const uint32_t unspecifiedIndex = nextNode.m_nodeIndex + GetNode(nextNode.m_nodeIndex).GetOffset();

                    // All the specified value nodes follow the unspecified node.
                    // The index of the requested node is calculated using the order of the option value.
                    const uint32_t requestedIndex = unspecifiedIndex + (optionValue + 1);

                    // If no option value was requested, this index is the same as the unspecified index.
                    if (requestedIndex > unspecifiedIndex)
                    {
                        // Visit this specified node, and increase the weight of visiting the node by 1.
                        // [GFX TODO] [ATOM-3883] Improve the evaluation of visiting the variant search tree.
                        nodesToVisitNext.push({ nextNode.m_branchCount + 1, requestedIndex });

                        // If the specified node has valid data, add it to the matches.
                        if (GetNode(requestedIndex).GetStableId().IsValid())
                        {
                            // Specified nodes have one more static branch than their parent.
                            searchResults.push_back({ nextNode.m_branchCount + 1, GetNode(requestedIndex).GetStableId() });
                        }
                    }

                    // Always visit the unspecified node.
                    nodesToVisitNext.push({ nextNode.m_branchCount, unspecifiedIndex });

                    // If the unspecified node has valid data, add it to the matches.
                    if (GetNode(unspecifiedIndex).GetStableId().IsValid())
                    {
                        // Unspecified nodes have the same number of static branches as their parent.
                        searchResults.push_back({ nextNode.m_branchCount, GetNode(unspecifiedIndex).GetStableId() });
                    }
                }

                // Visit the next nodes.
                AZStd::swap(nodesToVisit, nodesToVisitNext);
            }

            // Count the number of static branches.
            uint32_t totalBranchCount = 0;
            ShaderVariantStableId bestFitStableId = ShaderAsset::RootShaderVariantStableId;

            AZStd::for_each(searchResults.begin(), searchResults.end(), [&](const SearchResult& searchResult)
                {
                    // More static branches is a better fit.
                    if (searchResult.m_branchCount > totalBranchCount)
                    {
                        totalBranchCount = searchResult.m_branchCount;
                        bestFitStableId = searchResult.m_variantStableId;
                    }
                });

            // Calculate the number of dynamic branches. 
            const uint32_t optionCount = aznumeric_cast<uint32_t>(shaderOptionGroupLayout->GetShaderOptions().size());
            return ShaderVariantSearchResult{ bestFitStableId, optionCount - totalBranchCount };
        }

        const ShaderVariantTreeNode& ShaderVariantTreeAsset::GetNode(uint32_t index) const
        {
            AZ_Assert(index < m_nodes.size(), "Invalid Node Index");

            return m_nodes[index];
        }

        void ShaderVariantTreeAsset::SetNode(uint32_t index, const ShaderVariantTreeNode& node)
        {
            AZ_Assert(index < m_nodes.size(), "Invalid Node Index");

            m_nodes[index] = node;
        }

        AZStd::vector<uint32_t> ShaderVariantTreeAsset::ConvertToValueChain(const ShaderOptionGroupLayout* shaderOptionGroupLayout, const ShaderVariantId& shaderVariantId)
        {
            const auto& options = shaderOptionGroupLayout->GetShaderOptions();

            AZStd::vector<uint32_t> optionValues;
            optionValues.reserve(options.size());

            for (const ShaderOptionDescriptor& option : options)
            {
                if ((shaderVariantId.m_mask & option.GetBitMask()).any())
                {
                    optionValues.push_back(option.DecodeBits(shaderVariantId.m_key));
                }
                else
                {
                    optionValues.push_back(UnspecifiedIndex);
                }
            }

            // Remove trailing unspecified option values as they do not contribute anything to the search.
            while (!optionValues.empty() && optionValues.back() == UnspecifiedIndex)
            {
                optionValues.pop_back();
            }

            return optionValues;
        }

        void ShaderVariantTreeAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        bool ShaderVariantTreeAsset::FinalizeAfterLoad()
        {
            return true;
        }
         
        ShaderVariantTreeAssetHandler::LoadResult ShaderVariantTreeAssetHandler::LoadAssetData(const Data::Asset<Data::AssetData>& asset, AZStd::shared_ptr<Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            if (Base::LoadAssetData(asset, stream, assetLoadFilterCB) == LoadResult::LoadComplete)
            {
                return PostLoadInit(asset) ? LoadResult::LoadComplete : LoadResult::Error;
            }
            return LoadResult::Error;
        }


        bool ShaderVariantTreeAssetHandler::PostLoadInit(const Data::Asset<Data::AssetData>& asset)
        {
            if (ShaderVariantTreeAsset* shaderAsset = asset.GetAs<ShaderVariantTreeAsset>())
            {
                if (!shaderAsset->FinalizeAfterLoad())
                {
                    AZ_Error("ShaderVariantTreeAssetHandler", false, "Shader asset failed to finalize.");
                    return false;
                }
                return true;
            }
            return false;
        }

        void ShaderVariantTreeNode::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantTreeNode>()
                    ->Version(0)
                    ->Field("StableId", &ShaderVariantTreeNode::m_stableId)
                    ->Field("Offset", &ShaderVariantTreeNode::m_offset)
                    ;
            }
        }

        ShaderVariantTreeNode::ShaderVariantTreeNode()
            : m_stableId(ShaderVariantStableId{ ShaderVariantTreeAsset::UnspecifiedIndex })
            , m_offset(0)
        {

        }

        ShaderVariantTreeNode::ShaderVariantTreeNode(const ShaderVariantStableId& stableId, uint32_t offset)
            : m_stableId(stableId)
            , m_offset(offset)
        {

        }

        const ShaderVariantStableId& ShaderVariantTreeNode::GetStableId() const
        {
            return m_stableId;
        }

        uint32_t ShaderVariantTreeNode::GetOffset() const
        {
            return m_offset;
        }

        bool ShaderVariantTreeNode::HasChildren() const
        {
            return m_offset != 0;
        }
    } // namespace RPI
} // namespace AZ
