/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Edit/Shader/ShaderVariantTreeAssetCreator.h>

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

namespace AZ
{
    namespace RPI
    {
        // Arbitrary number to be reviewed that is used to constrain the range of options.
        static constexpr uint32_t MaxShaderVariantValues = 1000;

        AZ::Outcome<void, AZStd::string> ShaderVariantTreeAssetCreator::ValidateStableIdsAreUnique(const AZStd::vector<ShaderVariantListSourceData::VariantInfo>& shaderVariantList)
        {
            AZStd::unordered_map<ShaderVariantStableId, uint32_t> stableIdToIndexMap;
            stableIdToIndexMap.reserve(shaderVariantList.size());
            uint32_t sourceVariantIndex = 0;
            for (const ShaderVariantListSourceData::VariantInfo& variantInfo : shaderVariantList)
            {
                const ShaderVariantStableId variantInfoStableId{variantInfo.m_stableId};
                if (variantInfoStableId.IsNull() || variantInfoStableId == RootShaderVariantStableId)
                {
                    return AZ::Failure(AZStd::string::format("The variant at index=[%u] has StableId=[%u], which is forbidden.", sourceVariantIndex, variantInfoStableId.GetIndex()));
                }

                if (stableIdToIndexMap.find(variantInfoStableId) != stableIdToIndexMap.end())
                {
                    const uint32_t existingVariantIndex = stableIdToIndexMap.at(variantInfoStableId);
                    return AZ::Failure(AZStd::string::format("The variant at index=[%u] is trying to use StableId=[%u] which is already taken by variant at index=[%u]"
                        , sourceVariantIndex, variantInfoStableId.GetIndex(), existingVariantIndex));
                }
                stableIdToIndexMap.emplace(variantInfoStableId, sourceVariantIndex);
                sourceVariantIndex++;
            }
            return AZ::Success();
        }

        void ShaderVariantTreeAssetCreator::Begin(const AZ::Data::AssetId& assetId)
        {
            BeginCommon(assetId);
        }

        void ShaderVariantTreeAssetCreator::SetShaderOptionGroupLayout(const RPI::ShaderOptionGroupLayout& shaderOptionGroupLayout)
        {
            if (ValidateIsReady())
            {
                m_shaderOptionGroupLayout = &shaderOptionGroupLayout;
            }
        }

        void ShaderVariantTreeAssetCreator::SetVariantInfos(const AZStd::vector<ShaderVariantListSourceData::VariantInfo>& variantInfos)
        {
            if (ValidateIsReady())
            {
                // Add +1 space for the root variant.
                m_variantInfos.reserve(variantInfos.size() + 1);
                // When building the tree it'll be important that the first variant in the list
                // is the root variant.
                m_variantInfos.push_back(ShaderVariantListSourceData::VariantInfo());
                for (const auto& variantInfo : variantInfos)
                {
                    m_variantInfos.push_back(variantInfo);
                }
            }
        }

        //! Finalizes and assigns ownership of the asset to result, if successful. 
        //! Otherwise false is returned and result is left untouched.
        bool ShaderVariantTreeAssetCreator::End(Data::Asset<ShaderVariantTreeAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (!m_shaderOptionGroupLayout)
            {
                ReportError("No ShaderOptionGroupLayout has been set. Failed to finalize the ShaderVariantTreeAsset.");
                return false;
            }

            if (m_variantInfos.size() == 0)
            {
                ReportError("The list of source variants is not valid. Failed to finalize the ShaderVariantTreeAsset.");
                return false;
            }

            if (!EndInternal(result))
            {
                return false;
            }

            if (!m_asset->FinalizeAfterLoad())
            {
                ReportError("Failed to finalize the ShaderVariantTreeAsset.");
                return false;
            }

            m_asset->SetReady();
            return EndCommon(result);
        }

        bool ShaderVariantTreeAssetCreator::EndInternal([[maybe_unused]] Data::Asset<ShaderVariantTreeAsset>& result)
        {
            // Temporary structure used for sorting and caching intermediate results
            struct OptionCache
            {
                AZ::Name m_optionName;
                AZ::Name m_valueName;
                RPI::ShaderOptionIndex m_optionIndex; // Cached m_optionName
                RPI::ShaderOptionValue m_value;  // Cached m_valueName
            };
            AZStd::vector<OptionCache> optionList;
            // We can not have more options than the number of options in the layout:
            optionList.reserve(m_shaderOptionGroupLayout->GetShaderOptionCount());

            //Build the list of ShaderVariantId.
            AZStd::vector<ShaderVariantIdWithStableId> shaderVariantIds;
            shaderVariantIds.reserve(m_variantInfos.size());
            for (const ShaderVariantListSourceData::VariantInfo& variantInfo : m_variantInfos)
            {
                // Variants have their own set of option values so we rebuild the list for each variant:
                optionList.clear();

                // This loop will validate and cache the indices for each option value:
                for (const auto& shaderOption : variantInfo.m_options)
                {
                    Name optionName{ shaderOption.first };
                    Name optionValue{ shaderOption.second };

                    auto optionIndex = m_shaderOptionGroupLayout->FindShaderOptionIndex(optionName);
                    if (optionIndex.IsNull())
                    {
                        ReportError("Invalid shader option: %s", optionName.GetCStr());
                        continue;
                    }

                    auto option = m_shaderOptionGroupLayout->GetShaderOption(optionIndex);
                    auto value = option.FindValue(optionValue);
                    if (value.IsNull())
                    {
                        ReportError("Invalid value (%s) for shader option: %s", optionValue.GetCStr(), optionName.GetCStr());
                        continue;
                    }

                    optionList.push_back(OptionCache{ optionName, optionValue, optionIndex, value });
                }

                // The user might supply the option values in any order. Sort them now:
                AZStd::sort(optionList.begin(), optionList.end()
                    , [](const OptionCache& left, const OptionCache& right)
                    {
                            // m_optionIndex is the cached index in the m_options vector (stored in the ShaderOptionGroupLayout)
                            // m_options has already been sorted so the index *is* the option priority:
                            return left.m_optionIndex < right.m_optionIndex;
                    }
                );

                RPI::ShaderOptionGroup optionGroup(m_shaderOptionGroupLayout);
                for (const auto& optionCache : optionList)
                {

                    auto option = m_shaderOptionGroupLayout->GetShaderOption(optionCache.m_optionIndex);

                    // Assign the option value specified in the variant:
                    option.Set(optionGroup, optionCache.m_value);
                }
                shaderVariantIds.push_back({optionGroup.GetShaderVariantId(), ShaderVariantStableId{variantInfo.m_stableId}});
            }

            return BuildTree(shaderVariantIds);
        }

        bool ShaderVariantTreeAssetCreator::BuildTree(const AZStd::vector<ShaderVariantIdWithStableId>& shaderVariantIdsWithStableId)
        {
            //! Helper struct to build a dynamically allocated tree. The tree is then serialized into an accelerated search structure
            struct TreeNode
            {
                ShaderVariantStableId m_variantStableId;
                AZStd::vector<AZStd::shared_ptr<TreeNode>> m_children;

                TreeNode()
                    : m_variantStableId(ShaderVariantStableId{ ShaderVariantTreeAsset::UnspecifiedIndex })
                {
                }

                TreeNode(const ShaderVariantStableId& variantStableId)
                    : m_variantStableId(variantStableId)
                {
                }

                //! Bakes a node into the variant search tree.
                //! position   The position in the flat vector array of the tree where the node needs to be baked
                //! nextFree   The position in the flat vector array of the tree where the next free nodes can start from
                //! node       The node to bake.
                //! tree       The tree to bake into. If null, the node is not baked and the number of nodes is returned.
                static uint32_t BuildNode(uint32_t position, uint32_t nextFree, TreeNode* node, ShaderVariantTreeAsset* tree = nullptr)
                {
                    AZ_Assert(position < nextFree, "Invalid position for the current node");

                    const uint32_t offsetToChildren = node->m_children.empty() ? 0 : nextFree - position;

                    uint32_t childIndex = nextFree;
                    nextFree += aznumeric_cast<uint32_t>(node->m_children.size());

                    for (const auto& child : node->m_children)
                    {
                        if (child)
                        {
                            nextFree = BuildNode(childIndex, nextFree, child.get(), tree);
                        }

                        childIndex++;
                    }

                    if (tree)
                    {
                        (*tree).SetNode(position, ShaderVariantTreeNode{ node->m_variantStableId, offsetToChildren });
                        node->m_children.clear();
                    }

                    return nextFree;
                }
            };

            const auto& options = m_shaderOptionGroupLayout->GetShaderOptions();

            // The first variant is always the root.
            auto treeRoot = AZStd::make_unique<TreeNode>();
            treeRoot->m_variantStableId = ShaderAsset::RootShaderVariantStableId;

            // We start from the next variant after the root.
            for (uint32_t variantIndex = 1u; variantIndex < shaderVariantIdsWithStableId.size(); variantIndex++)
            {
                const ShaderVariantIdWithStableId shaderVariantIdWithStableId = shaderVariantIdsWithStableId[variantIndex];
                auto optionValues = ShaderVariantTreeAsset::ConvertToValueChain(m_shaderOptionGroupLayout, shaderVariantIdWithStableId.m_shaderVariantId);
                auto treeNode = treeRoot.get();

                for (uint32_t optionIndex = 0; optionIndex < optionValues.size(); optionIndex++)
                {
                    const uint32_t optionValue = optionValues[optionIndex];
                    const ShaderOptionDescriptor& option = options[optionIndex];

                    // Validation for unsupported features of the variant tree:
                    // - Large range of integers
                    // - Enums with gaps in their values

                    if (option.GetValuesCount() > MaxShaderVariantValues)
                    {
                        ReportError("Large integer ranges are not supported.");
                        continue;
                    }

                    if (option.GetMaxValue().GetIndex() - option.GetMinValue().GetIndex() + 1 != option.GetValuesCount())
                    {
                        ReportError("Enums with gaps are not supported.");
                        continue;
                    }

                    // The first time we add all the children.
                    if (treeNode->m_children.empty())
                    {
                        treeNode->m_children.resize(option.GetValuesCount() + 1, nullptr);
                    }

                    // If the child node at the correct index is still missing, create it.
                    if (treeNode->m_children[optionValue + 1] == nullptr)
                    {
                        // The variant index of a non-leaf node is invalid.
                        treeNode->m_children[optionValue + 1] = AZStd::make_shared<TreeNode>();
                    }

                    // Visit the next node.
                    treeNode = treeNode->m_children[optionValue + 1].get();
                }

                // Set the variant index for the current node.
                treeNode->m_variantStableId = ShaderVariantStableId{ shaderVariantIdWithStableId.m_stableId };
            }

            // Calculate the total size of the tree, and construct it.

            const uint32_t treeSize = TreeNode::BuildNode(0, 1, treeRoot.get(), nullptr);

            m_asset->m_nodes = 
                AZStd::vector<ShaderVariantTreeNode>(treeSize, ShaderVariantTreeNode());

            TreeNode::BuildNode(0, 1, treeRoot.get(), m_asset.Get());

            return true;
        }

    } // namespace RPI
} // namespace AZ
