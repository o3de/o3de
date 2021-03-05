/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Cry_Geo.h> // Needed for CGFContent.h
#include <CGFContent.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IOriginRule.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Common/WorldMatrixExporter.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneViews = AZ::SceneAPI::Containers::Views;

        WorldMatrixExporter::WorldMatrixExporter()
            : m_cachedRootMatrix(MatrixType::CreateIdentity())
            , m_cachedGroup(nullptr)
            , m_cachedRootMatrixIsSet(false)
        {
            BindToCall(&WorldMatrixExporter::ProcessMeshGroup);
            BindToCall(&WorldMatrixExporter::ProcessNode);
        }

        void WorldMatrixExporter::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<WorldMatrixExporter, SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        SceneEvents::ProcessingResult WorldMatrixExporter::ProcessMeshGroup(ContainerExportContext& context)
        {
            if (context.m_phase != Phase::Construction)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            m_cachedGroup = &context.m_group;

            m_cachedRootMatrix = MatrixType::CreateIdentity();
            m_cachedRootMatrixIsSet = false;

            AZStd::shared_ptr<const SceneDataTypes::IOriginRule> rule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneDataTypes::IOriginRule>();
            if (rule)
            {
                if (rule->GetTranslation() != Vector3(0.0f, 0.0f, 0.0f) || !rule->GetRotation().IsIdentity())
                {
                    m_cachedRootMatrix = MatrixType::CreateFromQuaternionAndTranslation(rule->GetRotation(), rule->GetTranslation());
                    m_cachedRootMatrixIsSet = true;
                }

                if (rule->GetScale() != 1.0f)
                {
                    float scale = rule->GetScale();
                    m_cachedRootMatrix.MultiplyByScale(Vector3(scale, scale, scale));
                    m_cachedRootMatrixIsSet = true;
                }

                if (!rule->GetOriginNodeName().empty() && !rule->UseRootAsOrigin())
                {
                    const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
                    SceneContainers::SceneGraph::NodeIndex index = graph.Find(rule->GetOriginNodeName());
                    if (index.IsValid())
                    {
                        MatrixType worldMatrix = MatrixType::CreateIdentity();
                        if (ConcatenateMatricesUpwards(worldMatrix, graph.ConvertToHierarchyIterator(index), graph))
                        {
                            worldMatrix.InvertFull();
                            m_cachedRootMatrix *= worldMatrix;
                            m_cachedRootMatrixIsSet = true;
                        }
                    }
                }
                return m_cachedRootMatrixIsSet ? SceneEvents::ProcessingResult::Success : SceneEvents::ProcessingResult::Ignored;
            }
            else
            {
                return SceneEvents::ProcessingResult::Ignored;
            }
        }

        SceneEvents::ProcessingResult WorldMatrixExporter::ProcessNode(NodeExportContext& context)
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            MatrixType worldMatrix = MatrixType::CreateIdentity();

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            HierarchyStorageIterator nodeIterator = graph.ConvertToHierarchyIterator(context.m_nodeIndex);
            bool translated = ConcatenateMatricesUpwards(worldMatrix, nodeIterator, graph);

            AZ_Assert(m_cachedGroup == &context.m_group, "NodeExportContext doesn't belong to chain of previously called MeshGroupExportContext.");
            if (m_cachedRootMatrixIsSet)
            {
                worldMatrix = m_cachedRootMatrix * worldMatrix;
                translated = true;
            }

 

            //If we aren't merging nodes we need to put the transforms into the localTM
            //due to how the CGFSaver works inside the ResourceCompilerPC code. 
            if (!context.m_container.GetExportInfo()->bMergeAllNodes)
            {
                SceneAPIMatrixTypeToMatrix34(context.m_node.localTM, worldMatrix);
            }
            else
            {
                SceneAPIMatrixTypeToMatrix34(context.m_node.worldTM, worldMatrix);
            }
            context.m_node.bIdentityMatrix = !translated;

            return SceneEvents::ProcessingResult::Success;
        }

        bool WorldMatrixExporter::ConcatenateMatricesUpwards(MatrixType& transform, const HierarchyStorageIterator& nodeIterator, const SceneContainers::SceneGraph& graph) const
        {
            bool translated = false;

            auto view = SceneViews::MakeSceneGraphUpwardsView(graph, nodeIterator, graph.GetContentStorage().cbegin(), true);
            for (auto it = view.begin(); it != view.end(); ++it)
            {
                if (!(*it))
                {
                    continue;
                }

                const SceneDataTypes::ITransform* nodeTransform = azrtti_cast<const SceneDataTypes::ITransform*>(it->get());
                if (nodeTransform)
                {
                    transform = nodeTransform->GetMatrix() * transform;
                    translated = true;
                }
                else
                {
                    bool endPointTransform = MultiplyEndPointTransforms(transform, it.GetHierarchyIterator(), graph);
                    translated = translated || endPointTransform;
                }
            }
            return translated;
        }

        bool WorldMatrixExporter::MultiplyEndPointTransforms(MatrixType& transform, const HierarchyStorageIterator& nodeIterator, const SceneContainers::SceneGraph& graph) const
        {
            // If the translation is not an end point it means it's its own group as opposed to being
            //      a component of the parent, so only list end point children.
            auto view = SceneViews::MakeSceneGraphChildView<SceneViews::AcceptEndPointsOnly>(graph, nodeIterator, 
                graph.GetContentStorage().begin(), true);
            auto result = AZStd::find_if(view.begin(), view.end(), SceneContainers::DerivedTypeFilter<SceneDataTypes::ITransform>());
            if (result != view.end())
            {
                transform = azrtti_cast<const SceneDataTypes::ITransform*>(result->get())->GetMatrix() * transform;
                return true;
            }
            else
            {
                return false;
            }
        }

        void WorldMatrixExporter::SceneAPIMatrixTypeToMatrix34(Matrix34& out, const MatrixType& in) const
        {
            // Setting column instead of row because as of writing Matrix34 doesn't support adding
            //      full rows, as the translation has to be done separately.
            for (int column = 0; column < 4; ++column)
            {
                Vector3 data = in.GetColumn(column);
                out.SetColumn(column, Vec3(data.GetX(), data.GetY(), data.GetZ()));
            }
        }
    } // namespace RC
} // namespace AZ
