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

#include <Editor/Assets/ScriptCanvasAssetTracker.h>
#endif

class QGraphicsView;
class QVBoxLayout;

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        struct GraphTabMetadata
        {
            AZ::Data::AssetId m_assetId;
            QWidget* m_hostWidget = nullptr;
            QString m_tabName;
            Tracker::ScriptCanvasFileState m_fileState = Tracker::ScriptCanvasFileState::INVALID;
        };

        class GraphTabBar
            : public AzQtComponents::TabBar
            , public MemoryAssetNotificationBus::MultiHandler
        {
            Q_OBJECT

        public:

            GraphTabBar(QWidget* parent = nullptr);
            ~GraphTabBar() override = default;

            void AddGraphTab(const AZ::Data::AssetId& assetId);
            int InsertGraphTab(int tabIndex, const AZ::Data::AssetId& assetId);
            bool SelectTab(const AZ::Data::AssetId& assetId);

            void ConfigureTab(int tabIndex, AZ::Data::AssetId fileAssetId, const AZStd::string& tabName);

            int FindTab(const AZ::Data::AssetId& assetId) const;
            AZ::Data::AssetId FindAssetId(int tabIndex);

            void CloseTab(int index);
            void CloseAllTabs();

            void OnContextMenu(const QPoint& point);

            void mouseReleaseEvent(QMouseEvent* event) override;

            // MemoryAssetNotifications
            void OnFileStateChanged(Tracker::ScriptCanvasFileState fileState) override;
            ////

            // Updates the tab at the supplied index with the GraphTabMetadata
            // The host widget field of the tabMetadata is not used and will not overwrite the tab data
            void SetTabText(int tabIndex, const QString& path, Tracker::ScriptCanvasFileState fileState = Tracker::ScriptCanvasFileState::INVALID);

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
            
            void SetFileState(AZ::Data::AssetId assetId, Tracker::ScriptCanvasFileState fileState);

            int m_signalSaveOnChangeTo = -1;
        };
    }
}

Q_DECLARE_METATYPE(ScriptCanvasEditor::Widget::GraphTabMetadata);
