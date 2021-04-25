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
#pragma once

#if !defined(Q_MOC_RUN)
#include <QCompleter>
#include <QAbstractItemModel>
#include <QRegExp>
#include <QString>
#include <QSortFilterProxyModel>
#include <QTreeView>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <ScriptCanvas/Data/Data.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserModel;
    }
}

namespace ScriptCanvasEditor 
{
    class UnitTestBrowserFilterModel;
    struct UnitTestResult;

    class UnitTestTreeView
        : public AzToolsFramework::QTreeViewWithStateSaving
    {
        Q_OBJECT
    public:
        UnitTestTreeView(QWidget* parent);
        ~UnitTestTreeView();

        void SetSearchFilter(const QString& filter);

        friend class UnitTestDockWidget;

    protected:
        void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
        void leaveEvent(QEvent* ev) Q_DECL_OVERRIDE;
        
    private:
        AzToolsFramework::AssetBrowser::AssetBrowserModel* m_model;
        UnitTestBrowserFilterModel* m_filter;
    };
}
