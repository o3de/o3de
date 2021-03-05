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
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#endif

class QAction;
class QMenu;
class QMouseEvent;
class QToolButton;
class QStyleOption;
class QStyleOptionTab;

namespace AzQtComponents
{
    class DockBar;

    class AZ_QT_COMPONENTS_API DockTabBar
        : public TabBar
    {
        Q_OBJECT
        Q_PROPERTY(bool singleTabFillsWidth READ singleTabFillsWidth WRITE setSingleTabFillsWidth NOTIFY singleTabFillsWidthChanged)
    public:
        static int closeButtonOffsetForIndex(const QStyleOptionTab* option);

        explicit DockTabBar(QWidget* parent = nullptr);
        using TabBar::mouseMoveEvent;
        using TabBar::mouseReleaseEvent;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void finishDrag();
        QSize sizeHint() const override;

        bool singleTabFillsWidth() const { return m_singleTabFillsWidth; }
        void setSingleTabFillsWidth(bool singleTabFillsWidth);
        void setIsShowingWindowControls(bool show);

        QString tabText(int index) const;

    Q_SIGNALS:
        void closeTab(int index);
        void undockTab(int index);
        void singleTabFillsWidthChanged(bool singleTabFillsWidth);

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void tabLayoutChange() override;
        void tabInserted(int index) override;

    protected Q_SLOTS:
        void currentIndexChanged(int current);
        void closeTabGroup();

    private:
        QWidget* m_tabIndicatorUnderlay;
        QToolButton* m_leftButton;
        QToolButton* m_rightButton;
        QMenu* m_contextMenu;
        QAction* m_closeTabMenuAction;
        QAction* m_closeTabGroupMenuAction;
        QAction* m_undockTabMenuAction;
        QAction* m_undockTabGroupMenuAction;
        int m_menuActionTabIndex;
        bool m_singleTabFillsWidth;
        bool m_isShowingWindowControls = false;
    };
} // namespace AzQtComponents
