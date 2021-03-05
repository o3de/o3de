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

#include <QStandardItemModel>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTreeView>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

#include <Editor/PropertyWidgets/LODSceneGraphWidget.h>
#include <Editor/PropertyWidgets/PropertyWidgetAllocator.h>
#include <SceneAPIExt/Utilities/LODSelector.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(LODSceneGraphWidget, EMotionFX::PropertyWidgetAllocator, 0)

            LODSceneGraphWidget::LODSceneGraphWidget(const SceneContainers::Scene& scene, const SceneDataTypes::ISceneNodeSelectionList& targetList,
                QWidget* parent)
                : SceneGraphWidget(scene, targetList, parent)
                , m_hideUncheckableItem(false)
            {
                AZ_Error("EMotionFX", azrtti_istypeof<Data::LodNodeSelectionList>(m_targetList.get()), "LOD SceneGraph widget must be used on LodNodeSelectionList");

                // Store a local list of LOD node for this level. Then we can compare this list with the actual selected node list in LOD Rule.
                Data::LodNodeSelectionList* lodNodeList = azrtti_cast<Data::LodNodeSelectionList*>(m_targetList.get());
                const AZ::u32 lodRuleIndex = lodNodeList->GetLODLevel() - 1;
                Utilities::LODSelector::SelectLODNodes(scene, m_LODSelectionList, lodRuleIndex);
            }

            void LODSceneGraphWidget::HideUncheckableItem(bool hide)
            {
                m_hideUncheckableItem = hide;
            }

            bool LODSceneGraphWidget::IsFilteredType([[maybe_unused]] const AZStd::shared_ptr<const SceneDataTypes::IGraphObject>& object,
                SceneContainers::SceneGraph::NodeIndex index) const
            {
                // In the LOD specific version of SceneGraphWidget, we want to filter out nodes that aren't belong to this LOD Level.
                // Noted: We intentionally skip the filterType check in base class because we know only the filter type nodes should be included in mLODSelectionList.
                const SceneContainers::SceneGraph& graph = m_scene.GetGraph();
                const char* nodePath = graph.ConvertToNameIterator(index)->GetPath();
                if (m_LODSelectionList.ContainsNode(nodePath))
                {
                    return true;
                }

                return false;
            }

            void LODSceneGraphWidget::Build()
            {
                setUpdatesEnabled(false);
                QSignalBlocker blocker(m_treeModel.data());

                const SceneContainers::SceneGraph& graph = m_scene.GetGraph();
                m_selectedCount = 0;
                m_totalCount = 0;
                m_treeModel->clear();
                m_treeItems.clear();
                m_treeItems = AZStd::vector<QStandardItem*>(graph.GetNodeCount(), nullptr);

                if (m_checkableOption == CheckableOption::NoneCheckable)
                {
                    GetQCheckBox()->hide();
                }
                else
                {
                    GetQCheckBox()->show();
                }

                auto sceneGraphView = SceneContainers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());
                auto sceneGraphDownardsIteratorView = SceneContainers::Views::MakeSceneGraphDownwardsView<SceneContainers::Views::BreadthFirst>(
                    graph, graph.GetRoot(), sceneGraphView.begin(), true);

                // Some importer implementations may write an empty node to force collection all items under a common root
                //  If that is the case, we're going to skip it so we don't show the user an empty node root
                auto iterator = sceneGraphDownardsIteratorView.begin();
                if (iterator->first.GetPathLength() == 0 && !iterator->second)
                {
                    ++iterator;
                }

                for (; iterator != sceneGraphDownardsIteratorView.end(); ++iterator)
                {
                    SceneContainers::SceneGraph::HierarchyStorageConstIterator hierarchy = iterator.GetHierarchyIterator();
                    SceneContainers::SceneGraph::NodeIndex currentIndex = graph.ConvertToNodeIndex(hierarchy);
                    AZ_Assert(currentIndex.IsValid(), "While iterating through the Scene Graph an unexpected invalid entry was found.");
                    AZStd::shared_ptr<const SceneDataTypes::IGraphObject> currentItem = iterator->second;

                    if (hierarchy->IsEndPoint())
                    {
                        switch (m_endPointOption)
                        {
                        case EndPointOption::AlwaysShow:
                            break;
                        case EndPointOption::NeverShow:
                            continue;
                        case EndPointOption::OnlyShowFilterTypes:
                            if (IsFilteredType(currentItem, currentIndex))
                            {
                                break;
                            }
                            else
                            {
                                continue;
                            }
                        default:
                            AZ_Assert(false, "Unsupported type %i for end point option.", m_endPointOption);
                            break;
                        }
                    }

                    bool isCheckable = false;
                    switch (m_checkableOption)
                    {
                    case CheckableOption::AllCheckable:
                        isCheckable = true;
                        break;
                    case CheckableOption::NoneCheckable:
                        isCheckable = false;
                        break;
                    case CheckableOption::OnlyFilterTypesCheckable:
                        isCheckable = IsFilteredType(currentItem, currentIndex);
                        break;
                    default:
                        AZ_Assert(false, "Unsupported type %i for checkable option.", m_checkableOption);
                        isCheckable = false;
                        break;
                    }

                    // Add an option to skip the uncheckable item in tree widget.
                    if (m_hideUncheckableItem && !isCheckable)
                    {
                        continue;
                    }

                    QStandardItem* treeItem = BuildTreeItem(currentItem, iterator->first, isCheckable, hierarchy->IsEndPoint());
                    if (isCheckable)
                    {
                        if (IsSelected(iterator->first))
                        {
                            treeItem->setCheckState(Qt::CheckState::Checked);
                            m_selectedCount++;
                        }
                        m_totalCount++;
                    }

                    m_treeItems[currentIndex.AsNumber()] = treeItem;
                    SceneContainers::SceneGraph::NodeIndex parentIndex = graph.GetNodeParent(currentIndex);
                    if (parentIndex.IsValid() && m_treeItems[parentIndex.AsNumber()])
                    {
                        m_treeItems[parentIndex.AsNumber()]->appendRow(treeItem);
                    }
                    else
                    {
                        m_treeModel->appendRow(treeItem);
                    }
                }

                GetQTreeView()->expandAll();
                UpdateSelectAllStatus();

                setUpdatesEnabled(true);
            }
        }
    }
}
