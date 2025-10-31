/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>

#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            /// This class is the interface for the cloth rule (aka cloth modifier).
            /// It exposes functions to extract cloth data.
            class IClothRule
                : public IRule
            {
            public:
                AZ_RTTI(IClothRule, "{5185510A-50BF-418A-ACB4-1A9E014C7E43}", IRule);

                ~IClothRule() override = default;

                /// Returns the name of the mesh node inside the FBX that will be exported as cloth.
                virtual const AZStd::string& GetMeshNodeName() const = 0;

                /// Returns cloth data from the mesh node selected in the cloth rule.
                virtual AZStd::vector<AZ::Color> ExtractClothData(const Containers::SceneGraph& graph, const size_t numVertices) const = 0;

                /// Finds the cloth rule affecting a mesh node and extracts cloth data.
                /// @param graph The scene graph
                /// @param meshNodeIndex The index of the original, unoptimized mesh node
                /// @param numVertices The vertex count used when extracting the cloth data
                /// @param rules The rules for the scene, which may include the cloth rule
                static AZStd::vector<AZ::Color> FindClothData(
                    const AZ::SceneAPI::Containers::SceneGraph& graph,
                    const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& meshNodeIndex,
                    const size_t numVertices,
                    const AZ::SceneAPI::Containers::RuleContainer& rules)
                {
                    AZStd::vector<AZ::Color> clothData;

                    // Note: it is important the original mesh node index is used, not a generated optimized mesh node,
                    // because the cloth rule tracks the mesh node based on the original node paths
                    AZStd::string_view meshNodeName = graph.GetNodeName(meshNodeIndex).GetPath();

                    for (size_t ruleIndex = 0; ruleIndex < rules.GetRuleCount(); ++ruleIndex)
                    {
                        const IClothRule* clothRule = azrtti_cast<const IClothRule*>(rules.GetRule(ruleIndex).get());
                        if (!clothRule)
                        {
                            continue;
                        }

                        // Reached a cloth rule for this mesh node?
                        if (meshNodeName != clothRule->GetMeshNodeName())
                        {
                            continue;
                        }

                        // If there is already cloth data it means there is more than 1 cloth rule affecting the same mesh.
                        if (!clothData.empty())
                        {
                            AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow, "Different cloth rules chose the same mesh node, only using the first cloth rule.");
                            continue;
                        }

                        clothData = clothRule->ExtractClothData(graph, numVertices);
                    }

                    return clothData;
                }
            };
        } //namespace DataTypes
    } //namespace SceneAPI
}  // namespace AZ
