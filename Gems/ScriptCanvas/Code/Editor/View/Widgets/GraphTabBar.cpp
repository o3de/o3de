/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <QGraphicsView>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QVariant>
#include <QVBoxLayout>


#include "GraphTabBar.h"

#include <ScriptCanvas/Bus/RequestBus.h>
#include <Editor/View/Widgets/CanvasWidget.h>
#include <Editor/QtMetaTypes.h>

#include <ScriptCanvas/Asset/AssetDescription.h>

#include <Editor/Include/ScriptCanvas/Components/EditorGraph.h>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        ////////////////
        // GraphTabBar
        ////////////////

        GraphTabBar::GraphTabBar(QWidget* parent)
            : AzQtComponents::TabBar(parent)
        {
            setTabsClosable(true);
            setMovable(true);
            setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

            connect(this, &QTabBar::currentChanged, this, &GraphTabBar::currentChangedTab);

            setContextMenuPolicy(Qt::CustomContextMenu);
            connect(this, &QTabBar::customContextMenuRequested, this, &GraphTabBar::OnContextMenu);
        }

        void GraphTabBar::AddGraphTab(SourceHandle assetId, Tracker::ScriptCanvasFileState fileState)
        {
            InsertGraphTab(count(), assetId, fileState);
        }

        void GraphTabBar::ClearTabView(int tabIndex)
        {
            if (tabIndex < count())
            {
                if (QVariant tabDataVariant = tabData(tabIndex); tabDataVariant.isValid())
                {
                    GraphTabMetadata replacement = tabDataVariant.value<GraphTabMetadata>();
                    if (replacement.m_canvasWidget)
                    {
                        delete replacement.m_canvasWidget;
                        replacement.m_canvasWidget = nullptr;
                        tabDataVariant.setValue(replacement);
                        setTabData(tabIndex, tabDataVariant);
                    }
                }
            }
        }

        CanvasWidget* GraphTabBar::ModOrCreateTabView(int tabIndex)
        {
            if (tabIndex < count())
            {
                if (QVariant tabDataVariant = tabData(tabIndex); tabDataVariant.isValid())
                {
                    if (!tabDataVariant.value<GraphTabMetadata>().m_canvasWidget)
                    {
                        CanvasWidget* canvasWidget = new CanvasWidget(tabDataVariant.value<GraphTabMetadata>().m_assetId, this);
                        canvasWidget->SetDefaultBorderColor(ScriptCanvasEditor::SourceDescription::GetDisplayColor());
                        GraphTabMetadata replacement = tabDataVariant.value<GraphTabMetadata>();
                        replacement.m_canvasWidget = canvasWidget;
                        tabDataVariant.setValue(replacement);
                        setTabData(tabIndex, tabDataVariant);
                    }

                    return tabDataVariant.value<GraphTabMetadata>().m_canvasWidget;
                }
            }

            return nullptr;
        }

        CanvasWidget* GraphTabBar::ModTabView(int tabIndex)
        {
            if (tabIndex < count())
            {
                if (QVariant tabDataVariant = tabData(tabIndex); tabDataVariant.isValid())
                {
                    return tabDataVariant.value<GraphTabMetadata>().m_canvasWidget;
                }
            }

            return nullptr;
        }

        AZStd::optional<GraphTabMetadata> GraphTabBar::GetTabData(int tabIndex) const
        {
            if (tabIndex < count())
            {
                if (QVariant tabDataVariant = tabData(tabIndex); tabDataVariant.isValid())
                {
                    return tabDataVariant.value<GraphTabMetadata>();
                }
            }

            return AZStd::nullopt;
        }

        AZStd::optional<GraphTabMetadata> GraphTabBar::GetTabData(SourceHandle assetId) const
        {
            return GetTabData(FindTab(assetId));
        }

        void GraphTabBar::SetTabData(const GraphTabMetadata& metadata, int tabIndex)
        {
            if (tabIndex < count())
            {
                setTabData(tabIndex, QVariant::fromValue(metadata));
            }
        }

        void GraphTabBar::SetTabData(const GraphTabMetadata& metadata, SourceHandle assetId)
        {
            auto index = FindTab(assetId);
            auto replacement = GetTabData(assetId);

            if (index >= 0 && replacement)
            {
                replacement->m_assetId = assetId;
                SetTabData(metadata, index);
            }
        }

        int GraphTabBar::InsertGraphTab(int tabIndex, SourceHandle assetId, Tracker::ScriptCanvasFileState fileState)
        {
            if (!SelectTab(assetId))
            {
                QIcon tabIcon = QIcon(ScriptCanvasEditor::SourceDescription::GetIconPath());
                tabIndex = qobject_cast<AzQtComponents::TabWidget*>(parent())->insertTab(tabIndex, new QWidget(), tabIcon, "");
                GraphTabMetadata metaData;
                CanvasWidget* canvasWidget = new CanvasWidget(assetId, this);
                canvasWidget->SetDefaultBorderColor(SourceDescription::GetDisplayColor());
                metaData.m_canvasWidget = canvasWidget;
                metaData.m_assetId = assetId;
                metaData.m_fileState = fileState;
                
                AZStd::string tabName;
                AzFramework::StringFunc::Path::GetFileName(assetId.RelativePath().c_str(), tabName);

                SetTabText(tabIndex, tabName.c_str(), fileState);
                setTabData(tabIndex, QVariant::fromValue(metaData));
                return tabIndex;
            }

            return -1;
        }

        bool GraphTabBar::SelectTab(SourceHandle assetId)
        {
            int tabIndex = FindTab(assetId);
            if (-1 != tabIndex)
            {
                setCurrentIndex(tabIndex);
                return true;
            }

            return false;
        }

        int GraphTabBar::FindTab(SourceHandle assetId) const
        {
            for (int tabIndex = 0; tabIndex < count(); ++tabIndex)
            {
                QVariant tabDataVariant = tabData(tabIndex);
                if (tabDataVariant.isValid())
                {
                    auto tabAssetId = tabDataVariant.value<GraphTabMetadata>(); 
                    if (tabAssetId.m_assetId.AnyEquals(assetId))
                    {
                        return tabIndex;
                    }
                }
            }

            return -1;
        }

        int GraphTabBar::FindTab(ScriptCanvasEditor::GraphPtrConst graph) const
        {
            for (int tabIndex = 0; tabIndex < count(); ++tabIndex)
            {
                QVariant tabDataVariant = tabData(tabIndex);
                if (tabDataVariant.isValid())
                {
                    auto tabAssetId = tabDataVariant.value<GraphTabMetadata>();
                    if (tabAssetId.m_assetId.Get() == graph)
                    {
                        return tabIndex;
                    }
                }
            }

            return -1;
        }

        int GraphTabBar::FindSaveOverMatch(SourceHandle assetId) const
        {
            for (int tabIndex = 0; tabIndex < count(); ++tabIndex)
            {
                QVariant tabDataVariant = tabData(tabIndex);
                if (tabDataVariant.isValid())
                {
                    auto tabAssetId = tabDataVariant.value<GraphTabMetadata>();
                    if (tabAssetId.m_assetId.Get() != assetId.Get() && tabAssetId.m_assetId.PathEquals(assetId))
                    {
                        return tabIndex;
                    }
                }
            }

            return -1;
        }

        SourceHandle GraphTabBar::FindTabByPath(AZStd::string_view path) const
        {
            SourceHandle candidate;
            candidate = SourceHandle::MarkAbsolutePath(candidate, path);

            for (int index = 0; index < count(); ++index)
            {
                QVariant tabdata = tabData(index);
                if (tabdata.isValid())
                {
                    auto tabAssetId = tabdata.value<GraphTabMetadata>();
                    if (tabAssetId.m_assetId.AnyEquals(candidate))
                    {
                        return tabAssetId.m_assetId;
                    }
                }
            }

            return {};
        }

        SourceHandle GraphTabBar::FindAssetId(int tabIndex)
        {
            QVariant dataVariant = tabData(tabIndex);
            if (dataVariant.isValid())
            {
                auto tabAssetId = dataVariant.value<GraphTabMetadata>();
                return tabAssetId.m_assetId;
            }

            return SourceHandle();
        }

        ScriptCanvas::ScriptCanvasId GraphTabBar::FindScriptCanvasIdFromGraphCanvasId(const GraphCanvas::GraphId& graphCanvasGraphId) const
        {
            for (int index = 0; index < count(); ++index)
            {
                QVariant tabdata = tabData(index);
                if (tabdata.isValid())
                {
                    auto tabAssetId = tabdata.value<GraphTabMetadata>();
                    if (tabAssetId.m_assetId.IsGraphValid()
                    && tabAssetId.m_assetId.Get()->GetGraphCanvasGraphId() == graphCanvasGraphId)
                    {
                        return tabAssetId.m_assetId.Get()->GetScriptCanvasId();
                    }
                }
            }

            return ScriptCanvas::ScriptCanvasId{};
        }

        void GraphTabBar::CloseTab(int index)
        {
            if (index >= 0 && index < count())
            {
                qobject_cast<AzQtComponents::TabWidget*>(parent())->removeTab(index);
            }
        }

        void GraphTabBar::CloseAllTabs()
        {
            for (int i = count() - 1; i >= 0; --i)
            {
                Q_EMIT TabCloseNoButton(i);
            }
        }

        void GraphTabBar::OnContextMenu(const QPoint& point)
        {
            QPoint screenPoint = mapToGlobal(point);

            int tabIndex = tabAt(point);
            bool hasValidTab = (tabIndex >= 0);
            bool isModified = false;

            QVariant tabdata = tabData(tabIndex);
            if (tabdata.isValid())
            {
                auto tabAssetId = tabdata.value<GraphTabMetadata>();

                Tracker::ScriptCanvasFileState fileState = Tracker::ScriptCanvasFileState::INVALID;
                isModified = fileState == Tracker::ScriptCanvasFileState::NEW || fileState == Tracker::ScriptCanvasFileState::MODIFIED;
            }

            QMenu menu;

            QAction* saveAction = menu.addAction("Save");
            saveAction->setEnabled(hasValidTab && isModified);

            QAction* closeAction = menu.addAction("Close");
            closeAction->setEnabled(hasValidTab);

            QAction* closeAllAction = menu.addAction("Close All");

            QAction* closeAllButThis = menu.addAction("Close All But This");
            closeAllButThis->setEnabled(hasValidTab);

            menu.addSeparator();
            QAction* fullPathAction = menu.addAction("Copy Source Path To Clipboard");
            fullPathAction->setEnabled(hasValidTab);

            QAction* action = menu.exec(screenPoint);

            if (action)
            {
                if (action == saveAction)
                {
                    if (tabIndex != currentIndex())
                    {
                        m_signalSaveOnChangeTo = tabIndex;
                        setCurrentIndex(tabIndex);
                    }
                    else
                    {
                        Q_EMIT SaveTab(tabIndex);
                    }
                }
                else if (action == closeAction)
                {
                    tabCloseRequested(tabIndex);
                }
                else if (action == closeAllAction)
                {
                    Q_EMIT CloseAllTabsSignal();
                }
                else if (action == closeAllButThis)
                {
                    Q_EMIT CloseAllTabsButSignal(tabIndex);
                }
                else if (action == fullPathAction)
                {
                    Q_EMIT CopyPathToClipboard(tabIndex);
                }
            }
        }

        void GraphTabBar::mouseReleaseEvent(QMouseEvent* event)
        {
            if (event->button() == Qt::MidButton)
            {
                int tabIndex = tabAt(event->localPos().toPoint());

                if (tabIndex >= 0)
                {
                    tabCloseRequested(tabIndex);
                    return;
                }
            }

            AzQtComponents::TabBar::mouseReleaseEvent(event);
        }

        void GraphTabBar::SetTabText(int tabIndex, const QString& path, Tracker::ScriptCanvasFileState fileState)
        {
            QString safePath = path;
            if (path.endsWith("^") || path.endsWith("*"))
            {
                safePath.chop(1);
            }

            if (tabIndex >= 0 && tabIndex < count())
            {
                const char* fileStateTag = "";
                switch (fileState)
                {
                case Tracker::ScriptCanvasFileState::NEW:
                    fileStateTag = "^";
                    break;
                case Tracker::ScriptCanvasFileState::SOURCE_REMOVED:
                case Tracker::ScriptCanvasFileState::MODIFIED:
                    fileStateTag = "*";
                    break;
                default:
                    break;
                }

                setTabText(tabIndex, QString("%1%2").arg(safePath).arg(fileStateTag));
            }
        }

        void GraphTabBar::tabInserted(int index)
        {
            AzQtComponents::TabBar::tabInserted(index);

            Q_EMIT TabInserted(index);
        }

        void GraphTabBar::tabRemoved(int index)
        {
            AzQtComponents::TabBar::tabRemoved(index);

            Q_EMIT TabRemoved(index);
        }

        void GraphTabBar::UpdateFileState(const SourceHandle& assetId, Tracker::ScriptCanvasFileState fileState)
        {
            auto tabData = GetTabData(assetId);
            if (tabData && tabData->m_fileState != Tracker::ScriptCanvasFileState::NEW && tabData->m_fileState != fileState)
            {
                int index = FindTab(assetId);
                tabData->m_fileState = fileState;
                SetTabData(*tabData, assetId);
                SetTabText(index, tabText(index), fileState);

                if (index == currentIndex())
                {
                    Q_EMIT OnActiveFileStateChanged();
                }
            }
        }

        void GraphTabBar::currentChangedTab(int index)
        {
            if (index < 0)
            {
                return;
            }

            QVariant tabdata = tabData(index);
            if (!tabdata.isValid())
            {
                return;
            }

            auto assetId = tabdata.value<GraphTabMetadata>().m_assetId;

            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::OnChangeActiveGraphTab, assetId);

            if (m_signalSaveOnChangeTo >= 0 && m_signalSaveOnChangeTo == index)
            {
                m_signalSaveOnChangeTo = -1;
                Q_EMIT SaveTab(m_signalSaveOnChangeTo);
            }
        }

        #include <Editor/View/Widgets/moc_GraphTabBar.cpp>
    }
}
