/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Utilities
        {
            DataTypes::MatrixType BuildWorldTransform(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex nodeIndex)
            {
                DataTypes::MatrixType outTransform = DataTypes::MatrixType::Identity();
                while (nodeIndex.IsValid())
                {
                    auto view = Containers::Views::MakeSceneGraphChildView<Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex,
                        graph.GetContentStorage().begin(), true);
                    auto result = AZStd::find_if(view.begin(), view.end(), Containers::DerivedTypeFilter<DataTypes::ITransform>());
                    if (result != view.end())
                    {
                        // Check if the node has any child transform node
                        const DataTypes::MatrixType& azTransform = azrtti_cast<const DataTypes::ITransform*>(result->get())->GetMatrix();
                        outTransform = azTransform * outTransform;
                    }
                    else
                    {
                        // Check if the node itself is a transform node.
                        AZStd::shared_ptr<const DataTypes::ITransform> transformData = azrtti_cast<const DataTypes::ITransform*>(graph.GetNodeContent(nodeIndex));
                        if (transformData)
                        {
                            outTransform = transformData->GetMatrix() * outTransform;
                        }
                    }

                    if (graph.HasNodeParent(nodeIndex))
                    {
                        nodeIndex = graph.GetNodeParent(nodeIndex);
                    }
                    else
                    {
                        break;
                    }
                }

                return outTransform;
            }            

            Containers::SceneGraph::NodeIndex GetImmediateChildOfType(
                const Containers::SceneGraph& graph, const Containers::SceneGraph::NodeIndex& nodeIndex, const AZ::TypeId& typeId)
            {
                auto childIndex = graph.GetNodeChild(nodeIndex);
                while (childIndex.IsValid())
                {
                    const auto nodeContent = graph.GetNodeContent(childIndex);
                    if (nodeContent && azrtti_istypeof(typeId, nodeContent.get()))
                    {
                        break;
                    }
                    childIndex = graph.GetNodeSibling(childIndex);
                }

                return childIndex;
            }
        } // Utilities
    } // SceneAPI
} // AZ
