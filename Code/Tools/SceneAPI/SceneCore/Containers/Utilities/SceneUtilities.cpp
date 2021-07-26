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
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ICoordinateSystemRule.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Utilities
        {
            DataTypes::MatrixType ConcatenateMatricesUpwards(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex nodeIndex)
            {
                DataTypes::MatrixType outTransform = DataTypes::MatrixType::Identity();
                while (nodeIndex.IsValid())
                {
                    auto view = Containers::Views::MakeSceneGraphChildView<Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, graph.GetContentStorage().begin(), true);
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

            SCENE_CORE_API DataTypes::MatrixType DetermineWorldTransform(const Containers::Scene& scene, const Containers::SceneGraph::NodeIndex nodeIndex, const Containers::RuleContainer& ruleContainer)
            {
                auto coordinateSystemRule = ruleContainer.FindFirstByType<DataTypes::ICoordinateSystemRule>();
                if (coordinateSystemRule && coordinateSystemRule->GetUseAdvancedData())
                {
                    SceneAPI::DataTypes::MatrixType matrix = SceneAPI::DataTypes::MatrixType::CreateIdentity();
                    if (coordinateSystemRule->GetTranslation() != Vector3(0.0f, 0.0f, 0.0f) || !coordinateSystemRule->GetRotation().IsIdentity())
                    {
                        matrix = DataTypes::MatrixType::CreateFromQuaternionAndTranslation(coordinateSystemRule->GetRotation(), coordinateSystemRule->GetTranslation());
                    }
                    if (coordinateSystemRule->GetScale() != 1.0f)
                    {
                        float scale = coordinateSystemRule->GetScale();
                        matrix.MultiplyByScale(Vector3(scale));
                    }
                    if (!coordinateSystemRule->GetOriginNodeName().empty())
                    {
                        auto rootIndex = scene.GetGraph().Find(coordinateSystemRule->GetOriginNodeName());
                        if (rootIndex.IsValid())
                        {
                            auto worldMatrix = ConcatenateMatricesUpwards(scene.GetGraph(), rootIndex);
                            worldMatrix.InvertFull();
                            matrix *= worldMatrix;
                        }
                    }
                    return matrix;
                }
                return ConcatenateMatricesUpwards(scene.GetGraph(), nodeIndex);
            }
        } // Utilities
    } // SceneAPI
} // AZ
