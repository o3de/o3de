/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QLabel>
#include <QToolButton>
#include <AzCore/std/bind/bind.h>
#include <RowWidgets/ui_NodeTreeSelectionWidget.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneUI/RowWidgets/NodeTreeSelectionWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(NodeTreeSelectionWidget, SystemAllocator);

            NodeTreeSelectionWidget::NodeTreeSelectionWidget(QWidget* parent)
                : QWidget(parent)
                , ui(new Ui::NodeTreeSelectionWidget())
                , m_narrowSelection(false)
                , m_filterName("nodes")
            {
                ui->setupUi(this);

                ui->m_selectButton->setIcon(QIcon(":/SceneUI/Manifest/TreeIcon.png"));
                connect(ui->m_selectButton, &QToolButton::clicked, this, &NodeTreeSelectionWidget::SelectButtonClicked);
            }

            NodeTreeSelectionWidget::~NodeTreeSelectionWidget() = default;

            void NodeTreeSelectionWidget::SetList(const DataTypes::ISceneNodeSelectionList& list)
            {
                m_list = list.Copy();
            }

            void NodeTreeSelectionWidget::CopyListTo(DataTypes::ISceneNodeSelectionList& target)
            {
                if (m_list)
                {
                    m_list->CopyTo(target);
                }
            }
            
            void NodeTreeSelectionWidget::SetFilterName(const AZStd::string& name)
            {
                ui->m_selectButton->setToolTip(QString::asprintf("Select %s", name.c_str()));
                m_filterName = name;
            }
            
            void NodeTreeSelectionWidget::SetFilterName(AZStd::string&& name)
            {
                ui->m_selectButton->setToolTip(QString::asprintf("Select %s", name.c_str()));
                m_filterName = AZStd::move(name);
            }

            void NodeTreeSelectionWidget::AddFilterType(const Uuid& idProperty)
            {
                if (m_filterTypes.find(idProperty) == m_filterTypes.end())
                {
                    m_filterTypes.insert(idProperty);
                }
            }

            void NodeTreeSelectionWidget::AddFilterVirtualType(Crc32 name)
            {
                if (m_filterVirtualTypes.find(name) == m_filterVirtualTypes.end())
                {
                    m_filterVirtualTypes.insert(name);
                }
            }

            void NodeTreeSelectionWidget::UseNarrowSelection(bool enable)
            {
                m_narrowSelection = enable;
            }

            void NodeTreeSelectionWidget::SelectButtonClicked()
            {
                AZ_Assert(!m_treeWidget, "Node tree already active, NodeTreeSelectionWidget button pressed multiple times.");
                AZ_Assert(m_list, "Requested updating of selection list before it was set.");
                ManifestWidget* root = ManifestWidget::FindRoot(this);
                AZ_Assert(root, "NodeTreeSelectionWidget is not a child of a ManifestWidget.");
                if (!m_list || !root)
                {
                    return;
                }

                AZStd::shared_ptr<Containers::Scene> scene = root->GetScene();
                if (!scene)
                {
                    return;
                }

                OverlayWidgetButtonList buttons;
                
                OverlayWidgetButton acceptButton;
                acceptButton.m_text = "Select";
                acceptButton.m_callback = AZStd::bind(&NodeTreeSelectionWidget::ListChangesAccepted, this);
                acceptButton.m_triggersPop = true;

                OverlayWidgetButton cancelButton;
                cancelButton.m_text = "Cancel";
                cancelButton.m_callback = AZStd::bind(&NodeTreeSelectionWidget::ListChangesCanceled, this);
                cancelButton.m_triggersPop = true;
                cancelButton.m_isCloseButton = true;

                buttons.push_back(&acceptButton);
                buttons.push_back(&cancelButton);

                ResetNewTreeWidget(*scene);

                for (const Uuid& filterType : m_filterTypes)
                {
                    m_treeWidget->AddFilterType(filterType);
                }
                
                for (Crc32 virtualTypeName : m_filterVirtualTypes)
                {
                    m_treeWidget->AddVirtualFilterType(virtualTypeName);
                }

                if (m_narrowSelection)
                {
                    m_treeWidget->MakeCheckable(SceneGraphWidget::CheckableOption::OnlyFilterTypesCheckable);
                }
                
                m_treeWidget->Build();

                QLabel* label = new QLabel("Finish selecting nodes to continue editing settings.");
                label->setAlignment(Qt::AlignCenter);
                OverlayWidget::PushLayerToContainingOverlay(this, label, m_treeWidget.get(), "Select nodes", buttons);
            }

            void NodeTreeSelectionWidget::ResetNewTreeWidget(const Containers::Scene& scene)
            {
                m_treeWidget.reset(aznew SceneGraphWidget(scene, *m_list));
            }

            void NodeTreeSelectionWidget::ListChangesAccepted()
            {
                m_list = m_treeWidget->ClaimTargetList();
                m_treeWidget.reset();
                
                emit valueChanged();
            }

            void NodeTreeSelectionWidget::ListChangesCanceled()
            {
                m_treeWidget.reset();
            }

            void NodeTreeSelectionWidget::UpdateSelectionLabel()
            {
                if (m_list)
                {
                    size_t selected = CalculateSelectedCount();
                    size_t total = CalculateTotalCount();
                    AZ_Assert(selected <= total, "Selected count of nodes should not be greater than the total count");
                    
                    if (total == 0)
                    {
                        ui->m_statusLabel->setText("Default selection");
                    }
                    else if (selected == total)
                    {
                        ui->m_statusLabel->setText(QString::asprintf("All %s selected", m_filterName.c_str()));
                    }
                    else
                    {
                        ui->m_statusLabel->setText(
                            QString("%1 of %2 %3 selected").arg(selected).arg(total).arg(m_filterName.c_str()));
                    }
                }
                else
                {
                    ui->m_statusLabel->setText("No list assigned");
                }
            }

            size_t NodeTreeSelectionWidget::CalculateSelectedCount()
            {
                if (!m_list)
                {
                    return 0;
                }

                const ManifestWidget* root = ManifestWidget::FindRoot(this);
                AZ_Assert(root, "NodeTreeSelectionWidget is not a child of a ManifestWidget.");
                if (!m_list || !root)
                {
                    return 0;
                }
                const Containers::SceneGraph& graph = root->GetScene()->GetGraph();

                size_t result = 0;
                AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList> tempList(m_list->Copy());
                Utilities::SceneGraphSelector::UpdateNodeSelection(graph, *tempList);
                tempList->EnumerateSelectedNodes(
                    [&](const AZStd::string& nodeName)
                    {
                        Containers::SceneGraph::NodeIndex index = graph.Find(nodeName);
                        if (!index.IsValid())
                        {
                            return true;
                        }

                        AZStd::shared_ptr<const DataTypes::IGraphObject> object = graph.GetNodeContent(index);
                        if (!object)
                        {
                            return true;
                        }

                        if (m_filterTypes.empty() && m_filterVirtualTypes.empty())
                        {
                            result++;
                            return true;
                        }

                        bool foundType = false;
                        for (const Uuid& type : m_filterTypes)
                        {
                            if (object->RTTI_IsTypeOf(type))
                            {
                                result++;
                                foundType = true;
                                break;
                            }
                        }
                        if (foundType)
                        {
                            return true;
                        }

                        // Check if the object is one of the registered virtual types.
                        Events::GraphMetaInfo::VirtualTypesSet virtualTypes;
                        Events::GraphMetaInfoBus::Broadcast(
                            &Events::GraphMetaInfo::GetVirtualTypes, virtualTypes, *root->GetScene(), index);
                        for (Crc32 name : virtualTypes)
                        {
                            if (m_filterVirtualTypes.contains(name))
                            {
                                result++;
                                break;
                            }
                        }

                        return true;
                    });
                return result;
            }

            size_t NodeTreeSelectionWidget::CalculateTotalCount()
            {
                const ManifestWidget* root = ManifestWidget::FindRoot(this);
                AZ_Assert(root, "NodeTreeSelectionWidget is not a child of a ManifestWidget.");
                if (!m_list || !root)
                {
                    return 0;
                }
                const Containers::SceneGraph& graph = root->GetScene()->GetGraph();

                size_t total = 0;
                if (m_filterTypes.empty() && m_filterVirtualTypes.empty())
                {
                    Containers::SceneGraph::HierarchyStorageConstData view = graph.GetHierarchyStorage();
                    if (!graph.GetNodeContent(graph.GetRoot()) && graph.GetNodeName(graph.GetRoot()).GetPathLength() == 0)
                    {
                        view = Containers::SceneGraph::HierarchyStorageConstData(view.begin() + 1, view.end());
                    }

                    for (auto& it : view)
                    {
                        if (!it.IsEndPoint())
                        {
                            total++;
                        }
                    }
                }
                else
                {
                    for (auto it = graph.GetContentStorage().begin(); it != graph.GetContentStorage().end(); ++it)
                    {
                        if (!(*it))
                        {
                            continue;
                        }
                        
                        Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(it);

                        bool foundType = false;
                        for (const Uuid& type : m_filterTypes)
                        {
                            if ((*it)->RTTI_IsTypeOf(type))
                            {
                                total++;
                                foundType = true;
                                break;
                            }
                        }
                        if (foundType)
                        {
                            continue;
                        }
                        
                        // Check if the object is one of the registered virtual types.
                        Events::GraphMetaInfo::VirtualTypesSet virtualTypes;
                        Events::GraphMetaInfoBus::Broadcast(&Events::GraphMetaInfo::GetVirtualTypes, virtualTypes, *root->GetScene(), index);
                        for (Crc32 name : virtualTypes)
                        {
                            if (m_filterVirtualTypes.find(name) != m_filterVirtualTypes.end())
                            {
                                total++;
                                break;
                            }
                        }
                    }
                }
                return total;
            }
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ

#include <RowWidgets/moc_NodeTreeSelectionWidget.cpp>
