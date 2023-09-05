/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QStandardItemModel>
#include <SceneWidgets/ui_SceneGraphWidget.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/stack.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneUI/SceneWidgets/SceneGraphWidget.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(SceneGraphWidget, SystemAllocator);

            SceneGraphWidget::SceneGraphWidget(const Containers::Scene& scene, QWidget* parent)
                : QWidget(parent)
                , ui(new Ui::SceneGraphWidget())
                , m_treeModel(new QStandardItemModel())
                , m_scene(scene)
                , m_targetList(nullptr)
                , m_selectedCount(0)
                , m_totalCount(0)
                , m_endPointOption(EndPointOption::AlwaysShow)
                , m_checkableOption(CheckableOption::NoneCheckable)
            {
                SetupUI();
            }

            SceneGraphWidget::SceneGraphWidget(const Containers::Scene& scene, const DataTypes::ISceneNodeSelectionList& targetList,
                QWidget* parent)
                : QWidget(parent)
                , ui(new Ui::SceneGraphWidget())
                , m_treeModel(new QStandardItemModel())
                , m_scene(scene)
                , m_targetList(targetList.Copy())
                , m_selectedCount(0)
                , m_totalCount(0)
                , m_endPointOption(EndPointOption::OnlyShowFilterTypes)
                , m_checkableOption(CheckableOption::AllCheckable)
            {
                SetupUI();
            }

            SceneGraphWidget::~SceneGraphWidget() = default;

            AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList>&& SceneGraphWidget::ClaimTargetList()
            {
                return AZStd::move(m_targetList);
            }

            void SceneGraphWidget::IncludeEndPoints(EndPointOption option)
            {
                m_endPointOption = option;
            }

            void SceneGraphWidget::MakeCheckable(CheckableOption option)
            {
                m_checkableOption = option;
            }

            void SceneGraphWidget::AddFilterType(const Uuid& id)
            {
                if (m_filterTypes.find(id) == m_filterTypes.end())
                {
                    m_filterTypes.insert(id);
                }
            }

            void SceneGraphWidget::AddVirtualFilterType(Crc32 name)
            {
                if (m_filterVirtualTypes.find(name) == m_filterVirtualTypes.end())
                {
                    m_filterVirtualTypes.insert(name);
                }
            }

            void SceneGraphWidget::SetupUI()
            {
                ui->setupUi(this);
                ui->m_selectionTree->setHeaderHidden(true);
                ui->m_selectionTree->setModel(m_treeModel.data());

                connect(ui->m_selectAllCheckBox, &QCheckBox::stateChanged, this, &SceneGraphWidget::OnSelectAllCheckboxStateChanged);
                connect(m_treeModel.data(), &QStandardItemModel::itemChanged, this, &SceneGraphWidget::OnTreeItemStateChanged);
                connect(ui->m_selectionTree->selectionModel(), &QItemSelectionModel::currentChanged, this, &SceneGraphWidget::OnTreeItemChanged);
            }

            void SceneGraphWidget::Build()
            {
                setUpdatesEnabled(false);
                QSignalBlocker blocker(m_treeModel.data());

                const Containers::SceneGraph& graph = m_scene.GetGraph();
                m_selectedCount = 0;
                m_totalCount = 0;
                m_treeModel->clear();
                m_treeItems.clear();
                m_treeItems = AZStd::vector<QStandardItem*>(graph.GetNodeCount(), nullptr);

                if (m_checkableOption == CheckableOption::NoneCheckable)
                {
                    ui->m_selectAllCheckBox->hide();
                }
                else
                {
                    ui->m_selectAllCheckBox->show();
                }

                auto sceneGraphView = Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());
                auto sceneGraphDownardsIteratorView = Containers::Views::MakeSceneGraphDownwardsView<Containers::Views::BreadthFirst>(
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
                    Containers::SceneGraph::HierarchyStorageConstIterator hierarchy = iterator.GetHierarchyIterator();
                    Containers::SceneGraph::NodeIndex currentIndex = graph.ConvertToNodeIndex(hierarchy);
                    AZ_Assert(currentIndex.IsValid(), "While iterating through the Scene Graph an unexpected invalid entry was found.");
                    AZStd::shared_ptr<const DataTypes::IGraphObject> currentItem = iterator->second;

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

                    QStandardItem* treeItem = BuildTreeItem(currentItem, iterator->first, isCheckable, hierarchy->IsEndPoint());
                    if (isCheckable)
                    {
                        if (IsSelected(iterator->first, false))
                        {
                            treeItem->setCheckState(Qt::CheckState::Checked);
                            m_selectedCount++;
                        }
                        m_totalCount++;
                    }
                    
                    m_treeItems[currentIndex.AsNumber()] = treeItem;
                    Containers::SceneGraph::NodeIndex parentIndex = graph.GetNodeParent(currentIndex);
                    if (parentIndex.IsValid() && m_treeItems[parentIndex.AsNumber()])
                    {
                        m_treeItems[parentIndex.AsNumber()]->appendRow(treeItem);
                    }
                    else
                    {
                        m_treeModel->appendRow(treeItem);
                    }
                }

                ui->m_selectionTree->expandAll();
                UpdateSelectAllStatus();

                setUpdatesEnabled(true);
            }

            bool SceneGraphWidget::IsFilteredType(const AZStd::shared_ptr<const DataTypes::IGraphObject>& object, 
                Containers::SceneGraph::NodeIndex index) const
            {
                if (!object)
                {
                    return false;
                }

                for (const Uuid& id : m_filterTypes)
                {
                    if (object->RTTI_IsTypeOf(id))
                    {
                        return true;
                    }
                }

                if (!m_filterVirtualTypes.empty())
                {
                    Events::GraphMetaInfo::VirtualTypesSet virtualTypes;
                    EBUS_EVENT(Events::GraphMetaInfoBus, GetVirtualTypes, virtualTypes, m_scene, index);

                    for (Crc32 name : virtualTypes)
                    {
                        if (m_filterVirtualTypes.find(name) != m_filterVirtualTypes.end())
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            QStandardItem* SceneGraphWidget::BuildTreeItem(const AZStd::shared_ptr<const DataTypes::IGraphObject>& object, 
                const Containers::SceneGraph::Name& name, bool isCheckable, [[maybe_unused]] bool isEndPoint) const
            {
                QStandardItem* treeItem = new QStandardItem(name.GetName());
                treeItem->setData(QString(name.GetPath()));
                treeItem->setEditable(false);
                treeItem->setCheckable(isCheckable);
                if (object)
                {
                    AZStd::string toolTip;
                    EBUS_EVENT(Events::GraphMetaInfoBus, GetToolTip, toolTip, object.get());
                    if (toolTip.empty())
                    {
                        treeItem->setToolTip(QString::asprintf("%s\n<%s>", name.GetPath(), object->RTTI_GetTypeName()));
                    }
                    else
                    {
                        treeItem->setToolTip(QString::asprintf("%s\n\n%s", name.GetPath(), toolTip.c_str()));
                    }
                    
                    AZStd::string iconPath;
                    EBUS_EVENT(Events::GraphMetaInfoBus, GetIconPath, iconPath, object.get());
                    if (!iconPath.empty())
                    {
                        treeItem->setIcon(QIcon(iconPath.c_str()));
                    }
                }
                
                return treeItem;
            }

            void SceneGraphWidget::OnSelectAllCheckboxStateChanged()
            {
                setUpdatesEnabled(false);
                QSignalBlocker blocker(m_treeModel.data());
                Qt::CheckState state = ui->m_selectAllCheckBox->checkState();

                if (m_targetList)
                {
                    m_targetList->ClearSelectedNodes();
                    m_targetList->ClearUnselectedNodes();

                    for (QStandardItem* item : m_treeItems)
                    {
                        if (!item || !item->isCheckable())
                        {
                            continue;
                        }
                        
                        item->setCheckState(state);

                        QVariant itemData = item->data();
                        if (itemData.isValid())
                        {
                            AZStd::string fullName = itemData.toString().toUtf8().data();
                            if (state == Qt::CheckState::Unchecked)
                            {
                                m_targetList->RemoveSelectedNode(fullName);
                            }
                            else
                            {
                                m_targetList->AddSelectedNode(AZStd::move(fullName));
                            }
                        }
                    }
                }
                else
                {
                    for (QStandardItem* item : m_treeItems)
                    {
                        if (item && item->isCheckable())
                        {
                            item->setCheckState(state);
                        }
                    }
                }

                m_selectedCount = (state == Qt::CheckState::Unchecked) ? 0 : m_totalCount;
                UpdateSelectAllStatus();

                setUpdatesEnabled(true);
            }

            void SceneGraphWidget::OnTreeItemStateChanged(QStandardItem* item)
            {
                setUpdatesEnabled(false);
                QSignalBlocker blocker(m_treeModel.data());

                Qt::CheckState state = item->checkState();
                bool decrement = (state == Qt::CheckState::Unchecked);
                if (decrement)
                {
                    if (!RemoveSelection(item))
                    {
                        item->setCheckState(Qt::CheckState::Checked);
                        return;
                    }
                }
                else
                {
                    if (!AddSelection(item))
                    {
                        item->setCheckState(Qt::CheckState::Unchecked);
                        return;
                    }
                }

                AZStd::stack<QStandardItem*> children;
                int rowCount = item->rowCount();
                for (int index = 0; index < rowCount; ++index)
                {
                    children.push(item->child(index));
                }

                while (!children.empty())
                {
                    QStandardItem* current = children.top();
                    children.pop();

                    if (decrement)
                    {
                        if (current->checkState() != Qt::CheckState::Unchecked && RemoveSelection(current))
                        {
                            current->setCheckState(state);
                        }
                    }
                    else
                    {
                        if (current->checkState() == Qt::CheckState::Unchecked && AddSelection(current))
                        {
                            current->setCheckState(state);
                        }
                    }

                    int rowCount2 = current->rowCount();
                    for (int index = 0; index < rowCount2; ++index)
                    {
                        children.push(current->child(index));
                    }
                }

                UpdateSelectAllStatus();
                setUpdatesEnabled(true);
            }

            void SceneGraphWidget::OnTreeItemChanged(const QModelIndex& current, const QModelIndex& /*previous*/)
            {
                QStandardItem* item = m_treeModel->itemFromIndex(current);
                
                QVariant itemData = item->data();
                if (!itemData.isValid() || itemData.type() != QVariant::Type::String)
                {
                    return;
                }

                AZStd::string fullName = itemData.toString().toUtf8().data();
                AZ_TraceContext("Selected item", fullName);
                Containers::SceneGraph::NodeIndex nodeIndex = m_scene.GetGraph().Find(fullName);
                AZ_Assert(nodeIndex.IsValid(), "Invalid node added to tree.");
                if (!nodeIndex.IsValid())
                {
                    return;
                }
                
                Q_EMIT SelectionChanged(m_scene.GetGraph().GetNodeContent(nodeIndex));
            }

            void SceneGraphWidget::UpdateSelectAllStatus()
            {
                QSignalBlocker blocker(ui->m_selectAllCheckBox);
                if (m_selectedCount == m_totalCount)
                {
                    ui->m_selectAllCheckBox->setText("Unselect all");
                    ui->m_selectAllCheckBox->setCheckState(Qt::CheckState::Checked);
                }
                else
                {
                    ui->m_selectAllCheckBox->setText("Select all");
                    ui->m_selectAllCheckBox->setCheckState(Qt::CheckState::Unchecked);
                }
            }

            bool SceneGraphWidget::IsSelected(const Containers::SceneGraph::Name& name, bool updateNodeSelection) const
            {
                if (!m_targetList)
                {
                    return false;
                }

                if (updateNodeSelection)
                {
                    // Use a temp list to get a valid state of the UI here based on selected/unselected nodes
                    // We use the temp list so that the real list actually keeps track of the user's selection
                    // Since UpdateNodeSelection will modify selected/unselected node lists for us.
                    AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList> tempList(m_targetList->Copy());
                    Utilities::SceneGraphSelector::UpdateNodeSelection(m_scene.GetGraph(), *tempList);
                    return IsSelectedInSelectionList(name, *tempList);
                }
                else
                {
                    return IsSelectedInSelectionList(name, *m_targetList);
                }
            }

            bool SceneGraphWidget::IsSelectedInSelectionList(const Containers::SceneGraph::Name& name, const DataTypes::ISceneNodeSelectionList& targetList) const
            {
                return targetList.IsSelectedNode(name.GetPath());
            }

            bool SceneGraphWidget::AddSelection(const QStandardItem* item)
            {
                if (!m_targetList)
                {
                    return true;
                }

                QVariant itemData = item->data();
                if (!itemData.isValid() || itemData.type() != QVariant::Type::String)
                {
                    return false;
                }

                AZStd::string fullName = itemData.toString().toUtf8().data();
                AZ_TraceContext("Item for addition", fullName);
                Containers::SceneGraph::NodeIndex nodeIndex = m_scene.GetGraph().Find(fullName);
                AZ_Assert(nodeIndex.IsValid(), "Invalid node added to tree.");
                if (!nodeIndex.IsValid())
                {
                    return false;
                }

                m_targetList->AddSelectedNode(fullName);
                m_selectedCount++;
                AZ_Assert(m_selectedCount <= m_totalCount, "Selected node count exceeds available node count.");
                return true;
            }

            bool SceneGraphWidget::RemoveSelection(const QStandardItem* item)
            {
                if (!m_targetList)
                {
                    return true;
                }

                QVariant itemData = item->data();
                if (!itemData.isValid() || itemData.type() != QVariant::Type::String)
                {
                    return false;
                }

                AZStd::string fullName = itemData.toString().toUtf8().data();
                AZ_TraceContext("Item for removal", fullName);
                Containers::SceneGraph::NodeIndex nodeIndex = m_scene.GetGraph().Find(fullName);
                AZ_Assert(nodeIndex.IsValid(), "Invalid node removed from tree.");
                if (!nodeIndex.IsValid())
                {
                    return false;
                }

                m_targetList->RemoveSelectedNode(fullName);
                AZ_Assert(m_selectedCount > 0, "Selected node count can not be decremented below zero.");
                m_selectedCount--;
                return true;
            }

            QCheckBox* SceneGraphWidget::GetQCheckBox()
            {
                return ui->m_selectAllCheckBox;
            }

            QTreeView* SceneGraphWidget::GetQTreeView()
            {
                return ui->m_selectionTree;
            }

        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ

#include <SceneWidgets/moc_SceneGraphWidget.cpp>
