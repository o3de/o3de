/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/Command.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <Editor/Plugins/ColliderWidgets/JointPropertyWidget.h>
#include <Editor/SkeletonModel.h>
#include <Editor/SelectionProxyModel.h>
#include <Editor/InspectorBus.h>
#include <Editor/SkeletonSortFilterProxyModel.h>
#include <QTreeView>
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMotionFX
{
    class SkeletonOutlinerPlugin
        : public EMStudio::DockWidgetPlugin
        , private EMotionFX::SkeletonOutlinerRequestBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        enum {
            CLASS_ID = 0x00754155
        };

        SkeletonOutlinerPlugin();
        ~SkeletonOutlinerPlugin() override;

        // EMStudioPlugin overrides
        void Reflect(AZ::ReflectContext* context) override;
        const char* GetName() const override                { return "Skeleton Outliner"; }
        uint32 GetClassID() const override                  { return CLASS_ID; }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }
        EMStudioPlugin* Clone() const override              { return new SkeletonOutlinerPlugin(); }
        bool Init() override;

        // SkeletalOutlinerRequestBus overrides
        Node* GetSingleSelectedNode() override;
        QModelIndex GetSingleSelectedModelIndex() override;
        AZ::Outcome<QModelIndexList> GetSelectedRowIndices() override;
        SkeletonModel* GetModel() override;
        void DataChanged(const QModelIndex& modelIndex) override;
        void DataListChanged(const QModelIndexList& modelIndexList) override;

        JointPropertyWidget* m_propertyWidget = nullptr;

    private slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnTextFilterChanged(const QString& text);
        void OnTypeFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters);
        void OnEntered(const QModelIndex& index);
        void Reinit();

        void OnContextMenu(const QPoint& position);

    private:
        bool eventFilter(QObject* object, QEvent* event) override;

        QWidget*                                m_mainWidget;
        QLabel*                                 m_noSelectionLabel;

        AzQtComponents::FilteredSearchWidget*   m_searchWidget;
        QWidget*                                m_headerWidget;
        QTreeView*                              m_treeView;
        AZStd::unique_ptr<SkeletonModel>        m_skeletonModel;
        QModelIndexList                         m_selectedRows;
        SkeletonSortFilterProxyModel*           m_filterProxyModel;
        static constexpr int s_iconSize = 16;

        // Callbacks
        // Works for all commands that use the actor id as well as the joint name mixins
        MCORE_DEFINECOMMANDCALLBACK(DataChangedCallback);
        static bool DataChanged(AZ::u32 actorId, const AZStd::string& jointName);
        AZStd::vector<MCore::Command::Callback*> m_commandCallbacks;
    };
} // namespace EMotionFX
