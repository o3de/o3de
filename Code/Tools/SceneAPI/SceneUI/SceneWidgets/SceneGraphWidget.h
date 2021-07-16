/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <string>
#include <QWidget>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#endif

class QStandardItem; 
class QStandardItemModel;
class QCheckBox;
class QTreeView;

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IGraphObject;
            class ISceneNodeSelectionList;
        }

        namespace UI
        {
            // QT space
            namespace Ui
            {
                class SceneGraphWidget;
            }
            class SCENE_UI_API SceneGraphWidget
                : public QWidget
            {
            public:
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                // Sets default settings for the widget. Content will not be constructed until "Build" is called.
                SceneGraphWidget(const Containers::Scene& scene, QWidget* parent = nullptr);
                // Sets default settings for the widget. Content will not be constructed until "Build" is called.
                SceneGraphWidget(const Containers::Scene& scene, const DataTypes::ISceneNodeSelectionList& targetList, 
                    QWidget* parent = nullptr);
                ~SceneGraphWidget() override;

                AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList>&& ClaimTargetList();

                enum class EndPointOption
                {
                    AlwaysShow,         // End points are always shown.
                    NeverShow,          // End points are never shown.
                    OnlyShowFilterTypes // End points are only shown if its type is in the filter type list.
                };
                // Updates the tree to include/exclude end points. Call "BuildTree()" to rebuild the tree.
                virtual void IncludeEndPoints(EndPointOption option);
                
                enum class CheckableOption
                {
                    AllCheckable,               // All nodes in the tree can be checked.
                    NoneCheckable,              // No nodes can be checked.
                    OnlyFilterTypesCheckable    // Only nodes in the filter type list can be checked.
                };
                // Updates the tree to include/exclude check boxes and the primary selection. Call "BuildTree()" to rebuild the tree.
                virtual void MakeCheckable(CheckableOption option);
                // Add a type to filter for. Filter types are used to determine if a check box is added and/or to be shown if
                //      the type is an end point. See "IncludeEndPoints" and "MakeCheckable" for more details.
                //      Call "Build()" to rebuild the tree.
                virtual void AddFilterType(const Uuid& id);
                // Add a virtual type to filter for. Filter types are used to determine if a check box is added and/or to be shown if
                //      the type is an end point. See "IncludeEndPoints" and "MakeCheckable" for more details.
                //      Call "Build()" to rebuild the tree.
                virtual void AddVirtualFilterType(Crc32 name);
                
                // Constructs the widget's content. Call this after making one or more changes to the settings.
                virtual void Build();

            Q_SIGNALS:
                void SelectionChanged(AZStd::shared_ptr<const DataTypes::IGraphObject> item);

            protected:
                virtual void SetupUI();
                virtual bool IsFilteredType(const AZStd::shared_ptr<const DataTypes::IGraphObject>& object, 
                    Containers::SceneGraph::NodeIndex index) const;
                virtual QStandardItem* BuildTreeItem(const AZStd::shared_ptr<const DataTypes::IGraphObject>& object, 
                    const Containers::SceneGraph::Name& name, bool isCheckable, bool isEndPoint) const;

                virtual void OnSelectAllCheckboxStateChanged();
                virtual void OnTreeItemStateChanged(QStandardItem* item);
                virtual void OnTreeItemChanged(const QModelIndex& current, const QModelIndex& previous);

                virtual void UpdateSelectAllStatus();

                /// If you are calling this on a lot of elements in quick succession (like in a Build function), set 
                /// updateNodeSelection to false for increased performance. 
                virtual bool IsSelected(const Containers::SceneGraph::Name& name, bool updateNodeSelection = true) const;
                virtual bool AddSelection(const QStandardItem* item);
                virtual bool RemoveSelection(const QStandardItem* item);

                QCheckBox* GetQCheckBox();
                QTreeView* GetQTreeView();

                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                AZStd::vector<QStandardItem*> m_treeItems;
                AZStd::set<Uuid> m_filterTypes;
                AZStd::set<Crc32> m_filterVirtualTypes;
                QScopedPointer<Ui::SceneGraphWidget> ui;
                QScopedPointer<QStandardItemModel> m_treeModel;
                AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList> m_targetList;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
                const Containers::Scene& m_scene;

                size_t m_selectedCount;
                size_t m_totalCount;
                EndPointOption m_endPointOption;
                CheckableOption m_checkableOption;

            private:
                bool IsSelectedInSelectionList(const Containers::SceneGraph::Name& name, const DataTypes::ISceneNodeSelectionList& targetList) const;
            };
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ
