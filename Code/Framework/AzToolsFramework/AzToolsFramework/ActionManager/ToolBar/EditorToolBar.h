/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>

#include <QWidgetAction>

class QAction;
class QToolBar;
class QWidget;

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class ToolBarManagerInterface;

    //! Editor ToolBar class definitions.
    //! Wraps a QToolBar and provides additional functionality to handle and sort its items.
    class EditorToolBar
    {
    public:
        EditorToolBar();
        explicit EditorToolBar(const AZStd::string& name);

        static void Initialize();

        void AddSeparator(int sortKey);
        void AddAction(int sortKey, AZStd::string actionIdentifier);
        void AddWidget(int sortKey, QWidget* widget);
        
        // Returns the pointer to the ToolBar.
        QToolBar* GetToolBar();
        const QToolBar* GetToolBar() const;

    private:
        void RefreshToolBar();

        enum class ToolBarItemType
        {
            Action = 0,
            Separator,
            Widget
        };

        struct ToolBarItem
        {
            explicit ToolBarItem(ToolBarItemType type = ToolBarItemType::Separator, AZStd::string identifier = "");
            explicit ToolBarItem(QWidget* widget);

            ToolBarItemType m_type;

            AZStd::string m_identifier;
            QWidgetAction* m_widgetAction = nullptr;
        };

        QToolBar* m_toolBar = nullptr;
        AZStd::multimap<int, ToolBarItem> m_toolBarItems;

        inline static ActionManagerInterface* m_actionManagerInterface = nullptr;
        inline static ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
