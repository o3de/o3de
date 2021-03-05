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

#include "precompiled.h"

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

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        GraphTabBar::GraphTabBar(QWidget* parent /*= nullptr*/)
            : AzQtComponents::TabBar(parent)
        {
            setTabsClosable(true);
            setMovable(true);
            setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

            connect(this, &QTabBar::currentChanged, this, &GraphTabBar::currentChangedTab);

            setContextMenuPolicy(Qt::CustomContextMenu);
            connect(this, &QTabBar::customContextMenuRequested, this, &GraphTabBar::OnContextMenu);
        }

        void GraphTabBar::RemoveAllBars()
        {
            for (int i = count() - 1; i >= 0; --i)
            {
                Q_EMIT TabCloseNoButton(i);
            }
        }

        void GraphTabBar::SetTabText(int tabIndex, const QString& path, Tracker::ScriptCanvasFileState fileState)
        {
            if (tabIndex >= 0 && tabIndex < count())
            {
                const char* fileStateTag = "";
                switch (fileState)
                {
                case Tracker::ScriptCanvasFileState::NEW:
                    fileStateTag = "^";
                    break;
                case Tracker::ScriptCanvasFileState::MODIFIED:
                    fileStateTag = "*";
                    break;
                default:
                    break;
                }

                setTabText(tabIndex, QString("%1%2").arg(path).arg(fileStateTag));
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

        bool GraphTabBar::SelectTab(const AZ::Data::AssetId& assetId)
        {
            int tabIndex = FindTab(assetId);
            if (-1 != tabIndex)
            {
                setCurrentIndex(tabIndex);
                return true;
            }
            return false;
        }

        int GraphTabBar::FindTab(const AZ::Data::AssetId& assetId) const
        {
            for (int tabIndex = 0; tabIndex < count(); ++tabIndex)
            {
                QVariant tabDataVariant = tabData(tabIndex);
                if (tabDataVariant.isValid())
                {
                    auto tabAssetId = tabDataVariant.value<AZ::Data::AssetId>();
                    if (tabAssetId == assetId)
                    {
                        return tabIndex;
                    }
                }
            }
            return -1;
        }

        AZ::Data::AssetId GraphTabBar::FindAssetId(int tabIndex)
        {
            QVariant dataVariant = tabData(tabIndex);
            if (dataVariant.isValid())
            {
                auto tabAssetId = dataVariant.value<AZ::Data::AssetId>();
                return tabAssetId;
            }

            return AZ::Data::AssetId();
        }

        void GraphTabBar::AddGraphTab(const AZ::Data::AssetId& assetId)
        {
            InsertGraphTab(count(),  assetId);
        }

        int GraphTabBar::InsertGraphTab(int tabIndex, const AZ::Data::AssetId& assetId)
        {
            if (!SelectTab(assetId))
            {
                AZStd::shared_ptr<ScriptCanvasMemoryAsset> memoryAsset;
                AssetTrackerRequestBus::BroadcastResult(memoryAsset, &AssetTrackerRequests::GetAsset, assetId);

                if (memoryAsset)
                {                    
                    ScriptCanvas::AssetDescription assetDescription = memoryAsset->GetAsset().Get()->GetAssetDescription();

                    QIcon tabIcon = QIcon(assetDescription.GetIconPathImpl());
                    int newTabIndex = qobject_cast<AzQtComponents::TabWidget*>(parent())->insertTab(tabIndex, new QWidget(), tabIcon, "");

                    CanvasWidget* canvasWidget = memoryAsset->CreateView(this);

                    canvasWidget->SetDefaultBorderColor(assetDescription.GetDisplayColorImpl());

                    auto fileState = memoryAsset->GetFileState();

                    AZStd::string tabName;
                    AzFramework::StringFunc::Path::GetFileName(memoryAsset->GetAbsolutePath().c_str(), tabName);

                    // For opened graphs we need to use their file assetId
                    if (memoryAsset->GetFileAssetId().IsValid())
                    {
                        setTabData(newTabIndex, QVariant::fromValue(memoryAsset->GetFileAssetId()));
                    }
                    else
                    {
                        // new graphs will need to use their in-memory assetid which we'll need to update 
                        // upon saving the asset
                        setTabData(newTabIndex, QVariant::fromValue(memoryAsset->GetId()));
                    }

                    SetTabText(newTabIndex, tabName.data(), fileState);

                    return newTabIndex;
                }
            }

            return -1;
        }

        void GraphTabBar::currentChangedTab(int index)
        {
            if (index < 0)
            {
                return;
            }

            QVariant tab = tabData(index);
            if (!tab.isValid())
            {
                return;
            }

            auto assetId = tab.value<AZ::Data::AssetId>();

            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::OnChangeActiveGraphTab, assetId);

        }

        void GraphTabBar::CloseTab(int index)
        {
            if (index >= 0 && index < count())
            {
                QVariant tab = tabData(index);
                if (tab.isValid())
                {
                    auto tabAssetId = tab.value<AZ::Data::AssetId>();
                    AssetTrackerRequestBus::Broadcast(&AssetTrackerRequests::ClearView, tabAssetId);
                }
                qobject_cast<AzQtComponents::TabWidget*>(parent())->removeTab(index);
            }
        }

        void GraphTabBar::OnContextMenu(const QPoint& point)
        {
            QPoint screenPoint = mapToGlobal(point);

            int tabIndex = tabAt(point);
            bool hasValidTab = (tabIndex >= 0);
            bool isModified = false;

            QVariant tab = tabData(tabIndex);
            if (tab.isValid())
            {
                auto tabAssetId = tab.value<AZ::Data::AssetId>();

                Tracker::ScriptCanvasFileState fileState;
                AssetTrackerRequestBus::BroadcastResult(fileState , &AssetTrackerRequests::GetFileState, tabAssetId);

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
                    Q_EMIT SaveTab(tabIndex);
                }
                else if (action == closeAction)
                {
                    tabCloseRequested(tabIndex);
                }
                else if (action == closeAllAction)
                {
                    Q_EMIT CloseAllTabs();
                }
                else if (action == closeAllButThis)
                {
                    Q_EMIT CloseAllTabsBut(tabIndex);
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

        void GraphTabBar::SetFileState(AZ::Data::AssetId assetId, Tracker::ScriptCanvasFileState fileState)
        {
            int index = FindTab(assetId);

            if (index >= 0 && index < count())
            {
                QVariant tab = tabData(index);
                if (tab.isValid())
                {
                    auto tabAssetId = tab.value<AZ::Data::AssetId>();

                    AZStd::string tabName;
                    AssetTrackerRequestBus::BroadcastResult(tabName, &AssetTrackerRequests::GetTabName, tabAssetId);
                    SetTabText(index, tabName.c_str(), fileState);
                }
            }
        }

        #include <Editor/View/Widgets/moc_GraphTabBar.cpp>
    }
}
