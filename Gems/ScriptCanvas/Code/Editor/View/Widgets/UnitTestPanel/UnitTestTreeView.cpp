/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qaction.h>
#include <qevent.h>
#include <qheaderview.h>
#include <qitemselectionmodel.h>
#include <qscrollbar.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Script/ScriptAsset.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/UnitTestPanel/UnitTestTreeView.h>
#include <Editor/View/Widgets/UnitTestPanel/moc_UnitTestTreeView.cpp>

#include <Editor/View/Dialogs/ContainerWizard/ContainerWizard.h>

#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/QtMetaTypes.h>

#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <Editor/Model/UnitTestBrowserFilterModel.h>

namespace ScriptCanvasEditor
{
    using namespace AzToolsFramework::AssetBrowser;

    //////////////////////
    // UnitTestTreeView
    //////////////////////

    UnitTestTreeView::UnitTestTreeView(QWidget* parent)
        : AzToolsFramework::QTreeViewWithStateSaving(parent)
        , m_filter(new UnitTestBrowserFilterModel(parent))
    {
        AssetBrowserComponentRequestBus::BroadcastResult(m_model, &AssetBrowserComponentRequests::GetAssetBrowserModel);
        
        if (!m_model)
        {
            AZ_Error("ScriptCanvas", false, "Unable to setup UnitTest TreeView, asset browser model was not provided.");
        }
        else
        {
            m_filter->setSourceModel(m_model);
            m_filter->FilterSetup();

            setModel(m_filter);

            QAbstractItemView::setIconSize(QSize(14, 14));
            setMouseTracking(true);
        }
    }

    UnitTestTreeView::~UnitTestTreeView()
    {
    }

    void UnitTestTreeView::SetSearchFilter(const QString& filter)
    {
        clearSelection();
        m_filter->SetSearchFilter(filter);
        
        if (!filter.isEmpty())
        {
            expandAll();
        }
    }

    void UnitTestTreeView::mouseMoveEvent(QMouseEvent* event)
    {
        QModelIndex index = indexAt(event->pos());
        QModelIndex sourceIndex = m_filter->mapToSource(index);

        if (sourceIndex.isValid())
        {
            m_filter->SetHoveredIndex(sourceIndex);
        }
        else
        {
            m_filter->SetHoveredIndex(QModelIndex());
        }

        AzToolsFramework::QTreeViewWithStateSaving::mouseMoveEvent(event);
    }

    void UnitTestTreeView::leaveEvent([[maybe_unused]] QEvent* ev)
    {
        m_filter->SetHoveredIndex(QModelIndex());
    }
    
}
