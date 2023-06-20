/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR_IMPL(LODSceneGraphWidget, EMotionFX::PropertyWidgetAllocator)

            LODSceneGraphWidget::LODSceneGraphWidget(const SceneContainers::Scene& scene, const SceneDataTypes::ISceneNodeSelectionList& targetList,
                QWidget* parent)
                : SceneGraphWidget(scene, targetList, parent)
                , m_hideUncheckableItem(false)
            {
            }

            void LODSceneGraphWidget::HideUncheckableItem(bool hide)
            {
                m_hideUncheckableItem = hide;
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
                auto sceneGraphDownwardsIteratorView = SceneContainers::Views::MakeSceneGraphDownwardsView<SceneContainers::Views::BreadthFirst>(
                    graph, graph.GetRoot(), sceneGraphView.begin(), true);

                // Some importer implementations may write an empty node to force collection all items under a common root
                //  If that is the case, we're going to skip it so we don't show the user an empty node root
                auto startIterator = sceneGraphDownwardsIteratorView.begin();
                if (startIterator->first.GetPathLength() == 0 && !startIterator->second)
                {
                    ++startIterator;
                }

                // 1. First find all the items to add to this widget.
                AZStd::unordered_map<SceneContainers::SceneGraph::NodeIndex::IndexType, bool> itemsToAdd;
                for (auto iterator = startIterator; iterator != sceneGraphDownwardsIteratorView.end(); ++iterator)
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

                    itemsToAdd.emplace(currentIndex.AsNumber(), isCheckable);

                    // We want to add all parent item to this item as well.
                    SceneContainers::SceneGraph::NodeIndex parentIndex = graph.GetNodeParent(currentIndex);
                    while (parentIndex.IsValid())
                    {
                        if (itemsToAdd.contains(parentIndex.AsNumber()))
                        {
                            break;
                        }
                        itemsToAdd.emplace(parentIndex.AsNumber(), true);
                        parentIndex = graph.GetNodeParent(parentIndex);
                    }
                }

                // 2. Add all the items following the scene graph order.
                for (auto iterator = startIterator; iterator != sceneGraphDownwardsIteratorView.end(); ++iterator)
                {
                    SceneContainers::SceneGraph::HierarchyStorageConstIterator hierarchy = iterator.GetHierarchyIterator();
                    SceneContainers::SceneGraph::NodeIndex currentIndex = graph.ConvertToNodeIndex(hierarchy);
                    if (!itemsToAdd.contains(currentIndex.AsNumber()))
                    {
                        continue;
                    }

                    AZStd::shared_ptr<const SceneDataTypes::IGraphObject> currentItem = iterator->second;
                    const bool isCheckable = itemsToAdd[currentIndex.AsNumber()];
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
