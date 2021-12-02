/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <QMetaType>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>

#include <ScriptCanvas/Core/Core.h>
#include <Editor/Assets/ScriptCanvasAssetTracker.h>
#include <ScriptCanvas/Bus/RequestBus.h>

#endif

class QGraphicsView;
class QVBoxLayout;

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class CanvasWidget;

        struct GraphTabMetadata
        {
            SourceHandle m_assetId;
            QWidget* m_hostWidget = nullptr;
            CanvasWidget* m_canvasWidget = nullptr;
            Tracker::ScriptCanvasFileState m_fileState = Tracker::ScriptCanvasFileState::INVALID;
        };

        class GraphTabBar
            : public AzQtComponents::TabBar
        {
            Q_OBJECT

        public:

            GraphTabBar(QWidget* parent = nullptr);
            ~GraphTabBar() override = default;

            AZStd::optional<GraphTabMetadata> GetTabData(int index) const;
            AZStd::optional<GraphTabMetadata> GetTabData(ScriptCanvasEditor::SourceHandle assetId) const;
            void SetTabData(const GraphTabMetadata& data, int index);
            void SetTabData(const GraphTabMetadata& data, ScriptCanvasEditor::SourceHandle assetId);

            void AddGraphTab(ScriptCanvasEditor::SourceHandle assetId, Tracker::ScriptCanvasFileState fileState);
            void CloseTab(int index);
            void CloseAllTabs();

            int InsertGraphTab(int tabIndex, ScriptCanvasEditor::SourceHandle assetId, Tracker::ScriptCanvasFileState fileState);
            bool SelectTab(ScriptCanvasEditor::SourceHandle assetId);

            int FindTab(ScriptCanvasEditor::SourceHandle assetId) const;
            int FindTab(ScriptCanvasEditor::GraphPtrConst graph) const;
            int FindSaveOverMatch(ScriptCanvasEditor::SourceHandle assetId) const;
            ScriptCanvasEditor::SourceHandle FindTabByPath(AZStd::string_view path) const;
            ScriptCanvasEditor::SourceHandle FindAssetId(int tabIndex);
            ScriptCanvas::ScriptCanvasId FindScriptCanvasIdFromGraphCanvasId(const GraphCanvas::GraphId& graphCanvasGraphId) const;

            void ClearTabView(int tabIndex);
            CanvasWidget* ModOrCreateTabView(int tabIndex);
            CanvasWidget* ModTabView(int tabIndex);

            void OnContextMenu(const QPoint& point);

            void mouseReleaseEvent(QMouseEvent* event) override;

            // Updates the tab at the supplied index with the GraphTabMetadata
            // The host widget field of the tabMetadata is not used and will not overwrite the tab data
            void SetTabText(int tabIndex, const QString& path, Tracker::ScriptCanvasFileState fileState = Tracker::ScriptCanvasFileState::INVALID);

            void UpdateFileState(const ScriptCanvasEditor::SourceHandle& assetId, Tracker::ScriptCanvasFileState fileState);

        Q_SIGNALS:
            void TabInserted(int index);
            void TabRemoved(int index);
            // Emits a signal to close the tab which is distinct from pressing the close button the actual tab bar.
            // This allows handling of the close tab button being pressed different than the actual closing of the tab.
            // Pressing the close tab button will prompt the user to save file in tab if it is modified
            void TabCloseNoButton(int index); 

            void SaveTab(int index);
            void CloseAllTabsSignal();
            void CloseAllTabsButSignal(int index);
            void CopyPathToClipboard(int index);

            void OnActiveFileStateChanged();

        protected:
            void tabInserted(int index) override;
            void tabRemoved(int index) override;

        private:
            // Called when the selected tab changes
            void currentChangedTab(int index);
            
            int m_signalSaveOnChangeTo = -1;
        };
    }
}

Q_DECLARE_METATYPE(ScriptCanvasEditor::Widget::GraphTabMetadata);
